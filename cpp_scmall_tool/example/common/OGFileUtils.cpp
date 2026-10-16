#include <algorithm>
#include "OGFileUtils.h"
#include "unzip.h" 

OG_BEGIN

FileUtils* FileUtils::s_sharedFileUtils = nullptr;



 
//-------------------------------------FileUtils--------------------------

//判断是否是绝对路径
bool FileUtils::isAbsolutePath(const std::string& strPath) const
{
	if (
		(strPath.length() > 2 &&
		((strPath[0] >= 'a' && strPath[0] <= 'z') ||
			(strPath[0] >= 'A' && strPath[0] <= 'Z'))
			&& strPath[1] == ':')

		||
		(strPath[0] == '/' && strPath[1] == '/'))
	{
		return true;
	}
	return false;
}
//转UTF8
std::wstring StringUtf8ToWideChar(const std::string& strUtf8)
{
	std::wstring ret;
// 	if (!strUtf8.empty())
// 	{
// 		int nNum = MultiByteToWideChar(CP_UTF8, 0, strUtf8.c_str(), -1, nullptr, 0);
// 		if (nNum)
// 		{
// 			WCHAR* wideCharString = new WCHAR[nNum + 1];
// 			wideCharString[0] = 0;
// 
// 			nNum = MultiByteToWideChar(CP_UTF8, 0, strUtf8.c_str(), -1, wideCharString, nNum + 1);
// 
// 			ret = wideCharString;
// 			delete[] wideCharString;
// 		}
// 		else
// 		{
// 			OGLOG("Wrong convert to WideChar code:0x%x", GetLastError());
// 		}
// 	}
	return ret;
}
//检测文件状态
bool FileUtils::isFileExistInternal(const std::string& strFilePath)const
{

	if (strFilePath.empty())
	{
		return false;
	}

	std::string strPath = strFilePath;
	if (!isAbsolutePath(strPath))
	{ // Not absolute path, add the default root path at the beginning.
		strPath.insert(0, _defaultResRootPath);
	}
	// 尝试以二进制只读方式打开
	FILE *fp = fopen(strPath.c_str(), "rb");
	if (fp)
	{
		fclose(fp);
		return true;
	} 
// 	DWORD attr = GetFileAttributesW(StringUtf8ToWideChar(strPath).c_str());
// 	if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY))
// 		return false;   //  not a file

	return false;
}
std::string FileUtils::getFullPathForFilenameWithinDirectory(const std::string& directory, const std::string& filename)const
{
	// get directory+filename, safely adding '/' as necessary
	std::string ret = directory;
	if (directory.size() && directory[directory.size() - 1] != '/') {
		ret += '/';
	}
	ret += filename;
	// if the file doesn't exist, return an empty string
	if (!isFileExistInternal(ret)) {
		ret = "";
	}
	return ret;
}

std::string FileUtils::getPathForFilename(const std::string& filename, const std::string & resource_path)const
{
	std::string file = filename;
	std::string file_path = "";
	size_t pos = filename.find_last_of('/');
	if (pos != std::string::npos)
	{
		file_path = filename.substr(0, pos + 1);
		file = filename.substr(pos + 1);
	}

	// searchPath + file_path + resourceDirectory
	std::string path = resource_path;
	path += file_path;


	path = getFullPathForFilenameWithinDirectory(path, file);

	return path;
}

std::string FileUtils::fullPathForFilename(const std::string &filename)const
{


	if (filename.empty())
	{
		return "";
	}

	if (isAbsolutePath(filename))
	{
		return filename;
	}

	// Already Cached ?
	auto cacheIter = _fullPathCache.find(filename);
	if (cacheIter != _fullPathCache.end())
	{
		return cacheIter->second;
	}
	for (const auto& searchIt : _searchPathArray)
	{
		std::string fullpath = this->getPathForFilename(filename, searchIt);
		if (fullpath.size())
		{
			_fullPathCache.emplace(std::make_pair(filename, fullpath));
			return fullpath;
		}
	}

	return "";
}
// 
// void FileUtils::addData(const std::string &zipFilePath, const std::string &fileName,unsigned char *buf,int len) {
// 	Data *d = new Data;
// 	d->copy(buf, len);
// 	//设置ZIP文件名
// 	size_t zipPos = zipFilePath.find_last_of('/');
// 	std::string zip_file;
// 	if (zipPos!=std::string::npos)
// 	{
// 		zip_file = zipFilePath.substr(zipPos + 1, zipFilePath.size());
// 	}
// 	 
// 	//设置ZIP文件路径
// 
// 	std::string zipFile_path;
// 	if (zipPos != std::string::npos)
// 	{
// 		zipFile_path = zipFilePath.substr(0,zipPos + 1);
// 	}
// 	 
// 	//设置将解压的文件路径
// 	zipPos = fileName.find_last_of('/');
// 	std::string curfile_path;
// 	if (zipPos != std::string::npos)
// 	{
// 		curfile_path = fileName.substr(0,zipPos + 1);
// 	}
// 	 
// 	//设置将解压的文件名
// 	zipPos = fileName.find_last_of('/');
// 	std::string cur_fileName;
// 	if (zipPos != std::string::npos)
// 	{
// 		cur_fileName = fileName.substr(zipPos + 1, fileName.size());
// 	}
// 	 
// 	//_FileCache.push_back(d);
// }
// bool FileUtils::loadZip(const char *zipFile, const std::string & passwd) {
// 
// 	
// 	auto f = FileUtils::getInstance();
// 	auto path = f->fullPathForFilename(zipFile);
//  
// 	// Open the zip file
// 	unzFile zipfile = unzOpen(path.c_str());
// 	if (zipfile == NULL)
// 	{
// 		printf("%s: not found\n", "1");
// 		return false;
// 	}
// 
// 	// Get info about the zip file
// 	unz_global_info global_info;
// 	unz_file_info file_info;
// 	if (unzGetGlobalInfo(zipfile, &global_info) != UNZ_OK)
// 	{
// 		printf("could not read file global info\n");
// 		unzClose(zipfile);
// 		return false;
// 	}
// 	int err;
// 
// // 	if (passwd.empty())
// // 	{
// // 		err = unzOpenCurrentFile(zipfile);
// // 	}
// // 	else
// // 
// // 		err = unzOpenCurrentFilePassword(zipfile, passwd.c_str());
// // 
// // 	if (err != UNZ_OK)
// // 		printf("%d", err);
// // 	else
// // 		printf("ok");
// 
// 
// 	// Buffer to hold data read from the zip file.
// 	char read_buffer[READ_SIZE];
// 
// 	
// 
// 	// Loop to extract all files
// 	uLong i;
// 	for (i = 0; i < global_info.number_entry; ++i)
// 	{
// 		
// 
// 		// Get info about current file.
// 	 //   unz_file_info file_info;
// 		char filename[MAX_FILENAME];
// 		memset(filename,0,MAX_FILENAME);
// 		if (unzGetCurrentFileInfo(
// 			zipfile,
// 			&file_info,
// 			filename,
// 			MAX_FILENAME,
// 			NULL, 0, NULL, 0) != UNZ_OK)
// 		{
// 			printf("could not read file info\n");
// 			unzClose(zipfile);
// 			return false;
// 		}
// 
// 		unsigned char *fileData;
// 		int cur_pos = 0;
// 
// 		int fileLen = file_info.uncompressed_size;
// 
// // 		size_t pos = path.find_last_of('/');
// // 		string allPath = "";
// // 		if (pos != string::npos)
// // 		{
// // 			allPath = path.substr(0, pos + 1);
// // 			allPath += filename;
// // 			memset(filename, 0, MAX_FILENAME);
// // 			memcpy(filename, allPath.c_str(), allPath.size());
// // 		}
// 
// 		// Check if this entry is a directory or file.
// 		const size_t filename_length = strlen(filename);
// 
// 
// 		if (filename[filename_length - 1] == dir_delimter)
// 		{
// 			// Entry is a directory, so create it.
// 		//	printf("dir:%s\n", filename);
// 			//mkdir( filename,511);
// 
// 		//	CreateDirectory(filename, NULL);
// 		}
// 		else
// 		{
// 			//当是文件时
// 			// Entry is a file, so extract it.
// 			//printf("file:%s\n", filename);
// 			if (passwd.empty())
// 			{
// 				err = unzOpenCurrentFile(zipfile);
// 			}
// 			else
// 
// 				err = unzOpenCurrentFilePassword(zipfile, passwd.c_str());
// 
// 			if (err != UNZ_OK)
// 			{
// 				printf("could not open file\n");
// 				unzClose(zipfile);
// 				return false;
// 			}
// 
// 			
// 			fileData = new unsigned char[fileLen];
// 
// 
// 			// Open a file to write out the data.
// 	//		FILE *out = fopen(filename, "wb");
// // 			if (out == NULL)
// // 			{
// // 				printf("could not open destination file\n");
// // 				unzCloseCurrentFile(zipfile);
// // 				unzClose(zipfile);
// // 				return -1;
// // 			}
// 
// 			err = UNZ_OK;
// 			do
// 			{
// 				err = unzReadCurrentFile(zipfile, read_buffer, READ_SIZE);
// 				if (err < 0)
// 				{
// 					printf("error %d\n", err);
// 					unzCloseCurrentFile(zipfile);
// 					unzClose(zipfile);
// 					return false;
// 				}
// 
// 				// Write data to file.
// 				if (err > 0)
// 				{
// 					memcpy(fileData + cur_pos, read_buffer, err);
// 					cur_pos += err;
// 					//fwrite(read_buffer, err, 1, out); // You should check return of fwrite...
// 				}
// 			} while (err > 0);
// 
// 		
// 
// 			f->addData(path, filename, fileData, fileLen);
// 			delete[] fileData;
// 		//	fclose(out);
// 		}
// 
// 		unzCloseCurrentFile(zipfile);
// 
// 		// Go the the next entry listed in the zip file.
// 		if ((i + 1) < global_info.number_entry)
// 		{
// 			if (unzGoToNextFile(zipfile) != UNZ_OK)
// 			{
// 				printf("cound not read next file\n");
// 				unzClose(zipfile);
// 				return false;
// 			}
// 		}
// 	}
// 
// 	unzClose(zipfile);
// 	return true;
// }
// int FileUtils::decompressZip(const char *zipFile,const std::string & passwd) {
// 
// 
// 	auto f = FileUtils::getInstance();
// 
// 	auto path = f->fullPathForFilename(zipFile);
// 
// 	//如果不是有效文件
// 	if (path=="")
// 	{
// 		return -1;
// 	}
// 	// Open the zip file
// 	unzFile zipfile = unzOpen(path.c_str());
// 	if (zipfile == NULL)
// 	{
// 		printf("%s: not found\n", "1");
// 		return -1;
// 	}
// 
// 	// Get info about the zip file
// 	unz_global_info global_info;
// 	unz_file_info file_info;
// 	if (unzGetGlobalInfo(zipfile, &global_info) != UNZ_OK)
// 	{
// 		printf("could not read file global info\n");
// 		unzClose(zipfile);
// 		return -1;
// 	}
// 	int err = 0;
//  
// // 	if (passwd.empty())
// // 	{
// // 		err = unzOpenCurrentFile(zipfile);
// // 	}
// // 	else
// // 
// // 		err = unzOpenCurrentFilePassword(zipfile, passwd.c_str());
// // 
// // 	if (err != UNZ_OK)
// // 		printf("%d", err);
// // 	else
// // 		printf("ok");
// 
// 
// 	char read_buffer[READ_SIZE];
// 
// 	// Loop to extract all files
// 	uLong i;
// 	for (i = 0; i < global_info.number_entry; ++i)
// 	{
// 		// Get info about current file.
// 	 //   unz_file_info file_info;
// 		char filename[MAX_FILENAME];
// 		if (unzGetCurrentFileInfo(
// 			zipfile,
// 			&file_info,
// 			filename,
// 			MAX_FILENAME,
// 			NULL, 0, NULL, 0) != UNZ_OK)
// 		{
// 			printf("could not read file info\n");
// 			unzClose(zipfile);
// 			return -1;
// 		}
// 
// 		size_t pos = path.find_last_of('/');
// 		std::string allPath = "";
// 		if (pos != std::string::npos)
// 		{
// 			allPath = path.substr(0, pos + 1);
// 			allPath += filename;
// 			memset(filename, 0, MAX_FILENAME);
// 			memcpy(filename, allPath.c_str(), allPath.size());
// 		}
// 
// 		// Check if this entry is a directory or file.
// 		const size_t filename_length = strlen(filename); 
// 		if (filename[filename_length - 1] == dir_delimter)
// 		{
// 			// Entry is a directory, so create it.
// 			printf("dir:%s\n", filename);
// 			//mkdir( filename,511);
// 
// 			CreateDirectory(filename, NULL);
// 		}
// 		else
// 		{
// 			// Entry is a file, so extract it.
// 			printf("file:%s\n", filename);
// 			if (passwd.empty())
// 			{
// 				err = unzOpenCurrentFile(zipfile);
// 			}
// 			else
// 
// 				err = unzOpenCurrentFilePassword(zipfile, passwd.c_str());
// 
// 			if (err != UNZ_OK)
// 			{
// 				printf("could not open file\n");
// 				unzClose(zipfile);
// 				return -1;
// 			}
// 
// 			// Open a file to write out the data.
// 			FILE *out = fopen(filename, "wb");
// 			if (out == NULL)
// 			{
// 				printf("could not open destination file\n");
// 				unzCloseCurrentFile(zipfile);
// 				unzClose(zipfile);
// 				return -1;
// 			}
// 
// 			err = UNZ_OK;
// 			do
// 			{
// 				err = unzReadCurrentFile(zipfile, read_buffer, READ_SIZE);
// 				if (err < 0)
// 				{
// 					printf("error %d\n", err);
// 					unzCloseCurrentFile(zipfile);
// 					unzClose(zipfile);
// 					return -1;
// 				}
// 
// 				// Write data to file.
// 				if (err > 0)
// 				{
// 					fwrite(read_buffer, err, 1, out); // You should check return of fwrite...
// 				}
// 			} while (err > 0);
// 
// 			fclose(out);
// 		}
// 
// 		unzCloseCurrentFile(zipfile);
// 
// 		// Go the the next entry listed in the zip file.
// 		if ((i + 1) < global_info.number_entry)
// 		{
// 			if (unzGoToNextFile(zipfile) != UNZ_OK)
// 			{
// 				printf("cound not read next file\n");
// 				unzClose(zipfile);
// 				return -1;
// 			}
// 		}
// 	}
// 
// 	unzClose(zipfile);
// 
// 	return 0;
// }

std::string FileUtils::getSuitableFOpen(const std::string & filenameUtf8) const
{
	//OGASSERT(false, "getSuitableFOpen should be override by platform FileUtils");
	return filenameUtf8;
}

Data FileUtils::getDataFromFile(const std::string& filename) const
{
	Data d;
	getContents(filename, &d);
	return d;
}
std::string FileUtils::getStringFromFile(const std::string& filename) const
{
	std::string s;
	getContents(filename, &s);
	return s;
}
FileUtils::Status FileUtils::getContents(const std::string& filename, ResizableBuffer* buffer) const
{
	if (filename.empty())
		return Status::NotExists;

	auto fs = FileUtils::getInstance();

	std::string fullPath = fs->fullPathForFilename(filename);
	if (fullPath.empty())
		return Status::NotExists;

	std::string suitableFullPath = fs->getSuitableFOpen(fullPath);

	struct stat statBuf;
	if (stat(suitableFullPath.c_str(), &statBuf) == -1) {
		return Status::ReadFailed;
	}

	if (!(statBuf.st_mode & S_IFREG)) {
		return Status::NotRegularFileType;
	}

	FILE *fp = fopen(suitableFullPath.c_str(), "rb");
	if (!fp)
		return Status::OpenFailed;

	size_t size = statBuf.st_size;

	buffer->resize(size);
	size_t readsize = fread(buffer->buffer(), 1, size, fp);
	fclose(fp);

	if (readsize < size) {
		buffer->resize(readsize);
		return Status::ReadFailed;
	}

	return Status::OK;
}


bool FileUtils::isFileExist(const std::string& filename) const
{
	if (isAbsolutePath(filename))
	{
		return isFileExistInternal(filename);
	}
	else
	{
		std::string fullpath = fullPathForFilename(filename);
		if (fullpath.empty())
			return false;
		else
			return true;
	}
}
std::string FileUtils::getFileExtension(const std::string& filePath) const
{
	std::string fileExtension;
	size_t pos = filePath.find_last_of('.');
	if (pos != std::string::npos)
	{
		fileExtension = filePath.substr(pos, filePath.length());

		std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);
	}

	return fileExtension;
}


OG_END