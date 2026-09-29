


#if defined(_WIN32)
#  include <direct.h>
#  include <windows.h>
#else
#  include <sys/stat.h>
#  include <sys/types.h>
#  include <dirent.h>
#  include <unistd.h>
#endif

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>

#include "ZipHelper.h"
#include "zip.h"
#include "unzip.h"
#include "zlib.h"


OG_BEGIN

//namespace zip {

    // ============================================================
    // Helpers
    // ============================================================
    static void makeDirs(const std::string& path)
    {
        if (path.empty()) return;

        std::string tmp;
        for (size_t i = 0; i < path.size(); ++i)
        {
            tmp += path[i];
            if (path[i] == '/' || path[i] == '\\')
            {
#if defined(_WIN32)
                _mkdir(tmp.c_str());
#else
                mkdir(tmp.c_str(), 0755);
#endif
            }
        }
#if defined(_WIN32)
        _mkdir(tmp.c_str());
#else
        mkdir(tmp.c_str(), 0755);
#endif
    }

    static std::string joinPath(const std::string& a, const std::string& b)
    {
        if (a.empty()) return b;
        if (b.empty()) return a;
        char last = a[a.size() - 1];
        if (last == '/' || last == '\\') return a + b;
        return a + "/" + b;
    }

    // 递归收集文件
    static void collectFiles(
        const std::string& baseDir,
        std::vector<std::pair<std::string, std::string>>& out)
    {
#if defined(_WIN32)
        std::string search = baseDir + "\\*";
        WIN32_FIND_DATAA fd;
        HANDLE hFind = FindFirstFileA(search.c_str(), &fd);
        if (hFind == INVALID_HANDLE_VALUE) return;

        do
        {
            std::string name = fd.cFileName;
            if (name == "." || name == "..") continue;

            std::string full = joinPath(baseDir, name);
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                collectFiles(full, out);
            }
            else
            {
                std::string rel = full.substr(baseDir.size());
                if (!rel.empty() && (rel[0] == '/' || rel[0] == '\\'))
                    rel = rel.substr(1);
                out.push_back({ full, rel });
            }
        }
        while (FindNextFileA(hFind, &fd));
        FindClose(hFind);
#else
        DIR* dir = opendir(baseDir.c_str());
        if (!dir) return;

        struct dirent* ent;
        while ((ent = readdir(dir)) != nullptr)
        {
            std::string name = ent->d_name;
            if (name == "." || name == "..") continue;

            std::string full = joinPath(baseDir, name);
            struct stat st;
            if (stat(full.c_str(), &st) != 0) continue;

            if (S_ISDIR(st.st_mode))
            {
                collectFiles(full, out);
            }
            else if (S_ISREG(st.st_mode))
            {
                std::string rel = full.substr(baseDir.size());
                if (!rel.empty() && (rel[0] == '/' || rel[0] == '\\'))
                    rel = rel.substr(1);
                out.push_back({ full, rel });
            }
        }
        closedir(dir);
#endif
    }

    static bool isDir(const std::string& path)
    {
#if defined(_WIN32)
        DWORD attr = GetFileAttributesA(path.c_str());
        return (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY);
#else
        struct stat st;
        if (stat(path.c_str(), &st) != 0) return false;
        return S_ISDIR(st.st_mode);
#endif
    }

    static bool isFile(const std::string& path)
    {
#if defined(_WIN32)
        DWORD attr = GetFileAttributesA(path.c_str());
        return (attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY);
#else
        struct stat st;
        if (stat(path.c_str(), &st) != 0) return false;
        return S_ISREG(st.st_mode);
#endif
    }

    // 对整个文件做异或（原地）
	static bool xorFile(const std::string& path, const std::string& key)
	{
		if (key.empty()) return true;

		std::fstream fp(path, std::ios::in | std::ios::out | std::ios::binary);
		if (!fp) return false;

		fp.seekg(0, std::ios::end);
		std::streamoff size = fp.tellg();
		fp.seekg(0, std::ios::beg);

		const size_t bufSize = 64 * 1024;
		std::vector<char> buf(bufSize);
		std::streamoff done = 0;
		size_t klen = key.size();

		while (done < size)
		{
			size_t toRead = (size_t)(std::min)((std::streamoff)bufSize, size - done);
			fp.read(buf.data(), toRead);
			for (size_t i = 0; i < toRead; ++i)
				buf[i] ^= key[(done + i) % klen];

			fp.seekp(done, std::ios::beg);
			fp.write(buf.data(), toRead);
			done += toRead;
		}

		fp.close();
		return true;
	}

    // ============================================================
    // Compress
    // ============================================================
    bool compress(
        const std::vector<std::string>& srcPaths,
        const std::string& zfd,
        const std::string& password,
		const std::string& xorKey)
    {
        // 1. 收集所有文件
        std::vector<std::pair<std::string, std::string>> files;

        for (const auto& p : srcPaths)
        {
            if (isDir(p))
            {
                collectFiles(p, files);
            }
            else if (isFile(p))
            {
                size_t pos = p.find_last_of("/\\");
                std::string name = (pos == std::string::npos) ? p : p.substr(pos + 1);
                files.push_back({ p, name });
            }
        }

        if (files.empty())
        {
            printf("[ERROR] compress: no input files\n");
            return false;
        }

        // 2. 打开 zip
		/*::*/zipFile zf = zipOpen(zfd.c_str(), APPEND_STATUS_CREATE);
        if (!zf)
        {
            printf("[ERROR] compress: cannot create zip: %s\n", zfd.c_str());
            return false;
        }

        const char* pwd = password.empty() ? nullptr : password.c_str();

        // 3. 逐个写入
        for (auto& f : files)
        {
            const std::string& full = f.first;
            const std::string& rel  = f.second;

            std::ifstream in(full, std::ios::binary);
            if (!in)
            {
                printf("[ERROR] compress: cannot open %s\n", full.c_str());
                continue;
            }

            zip_fileinfo zi;
            memset(&zi, 0, sizeof(zi));

            int err = zipOpenNewFileInZip3(
                zf,
                rel.c_str(),
                &zi,
                nullptr, 0,
                nullptr, 0,
                nullptr,
                Z_DEFLATED,
                Z_DEFAULT_COMPRESSION,
                0,
                -MAX_WBITS,
                DEF_MEM_LEVEL,
                Z_DEFAULT_STRATEGY,
                pwd,
                pwd ? (uLong)strlen(pwd) : 0
            );

            if (err != ZIP_OK)
            {
                printf("[ERROR] compress: zipOpenNewFileInZip3 failed for %s (err=%d)\n",
                       rel.c_str(), err);
                continue;
            }

            const size_t bufSize = 64 * 1024;
            std::vector<char> buf(bufSize);
            while (in)
            {
                in.read(buf.data(), bufSize);
                std::streamsize rd = in.gcount();
                if (rd <= 0) break;
                zipWriteInFileInZip(zf, buf.data(), (unsigned int)rd);
            }

            zipCloseFileInZip(zf);
        }

        zipClose(zf, nullptr);

        // 4. 异或
        if (!xorKey.empty())
        {
            if (!xorFile(zfd, xorKey))
            {
                printf("[ERROR] compress: xor failed, key=%s\n", xorKey);
                return false;
            }
        }

        return true;
    }

    // ============================================================
    // Decompress
    // ============================================================
    bool decompress(
        const std::string& zipFile,
        const std::string& outDir,
        const std::string& password,
		const std::string& xorKey)
    {
		int failCount = 0;   // ← 加这行

        // 1. 如果指定异或，先复制一份并异或回原始 zip
        std::string realZip = zipFile;
        std::string tmpZip;

        if (!xorKey.empty())
        {
            tmpZip = zipFile + ".unxor.tmp";
            {
                std::ifstream in(zipFile, std::ios::binary);
                std::ofstream out(tmpZip, std::ios::binary);
                out << in.rdbuf();
            }
            if (!xorFile(tmpZip, xorKey))
            {
                printf("[ERROR] decompress: xor failed, key=%s\n", xorKey);
                return false;
            }
            realZip = tmpZip;
        }

        // 2. 打开 zip
        unzFile uf = unzOpen(realZip.c_str());
        if (!uf)
        {
            printf("[ERROR] decompress: cannot open zip: %s\n", realZip.c_str());
            printf("        possible reasons:\n");
            printf("        - file does not exist\n");
            printf("        - xor key is wrong\n");
            printf("        - not a valid zip\n");
            if (!tmpZip.empty()) remove(tmpZip.c_str());
            return false;
        }

        // 3. 全局信息
        unz_global_info gi;
        if (unzGetGlobalInfo(uf, &gi) != UNZ_OK)
        {
            printf("[ERROR] decompress: unzGetGlobalInfo failed\n");
            unzClose(uf);
            if (!tmpZip.empty()) remove(tmpZip.c_str());
            return false;
        }

        makeDirs(outDir);

        const char* pwd = password.empty() ? nullptr : password.c_str();

        // 4. 遍历
        for (uLong i = 0; i < gi.number_entry; ++i)
        {
            char filename[512] = { 0 };
            unz_file_info fi;
            if (unzGetCurrentFileInfo(uf, &fi, filename, sizeof(filename),
                nullptr, 0, nullptr, 0) != UNZ_OK)
            {
                printf("[ERROR] decompress: unzGetCurrentFileInfo failed at %u\n", (unsigned)i);
                break;
            }

            size_t nameLen = strlen(filename);
            std::string outPath = joinPath(outDir, filename);

            // 目录
            if (nameLen > 0 &&
                (filename[nameLen - 1] == '/' || filename[nameLen - 1] == '\\'))
            {
                makeDirs(outPath);
            }
            else
            {
                size_t pos = outPath.find_last_of("/\\");
                if (pos != std::string::npos)
                    makeDirs(outPath.substr(0, pos));

                int err = unzOpenCurrentFilePassword(uf, pwd);
                if (err != UNZ_OK)
                {
					failCount++;   // ← 加这行

                    if (err == UNZ_BADPASSWORD)
                        printf("[ERROR] decompress: wrong password for %s\n", filename);
                    else if (err == UNZ_BADZIPFILE)
                        printf("[ERROR] decompress: bad zip or wrong xor key: %s\n", filename);
                    else
                        printf("[ERROR] decompress: open file failed: %s (err=%d)\n", filename, err);
                }
                else
                {
                    std::ofstream out(outPath, std::ios::binary);
                    const size_t bufSize = 64 * 1024;
                    std::vector<char> buf(bufSize);
                    int rd = 0;
                    while ((rd = unzReadCurrentFile(uf, buf.data(), (unsigned)bufSize)) > 0)
                    {
                        out.write(buf.data(), rd);
                    }
                    if (rd < 0)
                    {
						failCount++;   // ← 加这行

                        if (rd == UNZ_CRCERROR)
                            printf("[ERROR] decompress: CRC error (corrupted or wrong password): %s\n", filename);
                        else
                            printf("[ERROR] decompress: read failed: %s (err=%d)\n", filename, rd);
                    }
                    out.close();
                    unzCloseCurrentFile(uf);
                }
            }

            if (i + 1 < gi.number_entry)
                unzGoToNextFile(uf);
        }

        unzClose(uf);

        if (!tmpZip.empty())
            remove(tmpZip.c_str());

		return failCount == 0;   // ← 改成这行
    }

//} // namespace zip


OG_END