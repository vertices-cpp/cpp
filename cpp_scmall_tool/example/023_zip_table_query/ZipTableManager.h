#ifndef _ZIP_TABLE_MANANGER_H_
#define _ZIP_TABLE_MANANGER_H_
/* 使用方法
auto ztb = ZipTableManager::getInstance();

// 1. 标准 zip（无加密、无密码）
ztb->addZip("res/base.zip");

// 2. 异或 zip（无密码）
ztb->addZip("res/xored.zip", "afd56f5a5dsa");

// 3.  密码 + 异或  
ztb->addZip("res/enc.zip", "afd56f5a5dsa", "123456");

// 4. 热更包（插到最前，优先级高）
ztb->addZipHighPriority("patch/hot.zip", "afd56f5a5dsa");
*/
// 1. 加回 _FILE_OFFSET_BITS（最前，include 之前）
#if !defined(_WIN32)
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#endif


#include "zlib.h"
#include "unzip.h"
#include "OGHeader.h"
#include "OGFileUtils.h"

OG_BEGIN

 

// ==================== 跨平台 64 位 IO（只留这一段） ====================
#if defined(_WIN32)
#define FILE_OPEN64(path, mode)  fopen((path), (mode))
#define FILE_SEEK64(fp, off, wh) _fseeki64((fp), (__int64)(off), (wh))
#define FILE_TELL64(fp)          _ftelli64((fp))
typedef __int64 plat_off_t;
#else
#define FILE_OPEN64(path, mode)  fopen((path), (mode))
#define FILE_SEEK64(fp, off, wh) fseeko((fp), (off_t)(off), (wh))
#define FILE_TELL64(fp)          ftello((fp))
typedef off_t plat_off_t;
#endif

// ==================== 路径归一化 ====================
// ZipTableManager 只接收逻辑路径（相对），这里只做基础归一化
// 绝对路径 → 逻辑路径，由上层 ResLoader 处理
static std::string normalizeKey(std::string p) {
	std::replace(p.begin(), p.end(), '\\', '/');
	while (p.rfind("./", 0) == 0) p = p.substr(2);
	return p;
}

// ==================== 单个 zip 归档 ====================
class ZipArchive {
public:

	struct Entry {
		unz64_file_pos pos;   // entry 在 zip 里的位置
		uint64_t rawSize;     // 解压后大小
		uint32_t crc32;       // 校验
		uint16_t method;      // 0=store, 8=deflate
		uint16_t flags;       // bit0=是否加密
	};

	// ---------- 五大 ----------
	// 含 std::mutex，不可拷贝不可移动
	// 外部必须用 shared_ptr / unique_ptr 持有
	ZipArchive() = default;
	~ZipArchive() { close(); }

	ZipArchive(const ZipArchive&) = delete;
	ZipArchive& operator=(const ZipArchive&) = delete;
	ZipArchive(ZipArchive&&) = delete;
	ZipArchive& operator=(ZipArchive&&) = delete;

	// ---------- 打开 ----------
	// xorKey 空 = 标准 zip
	// password 空 = 不加密
	bool open(const std::string& path,
		const std::string& password = "",
		const std::string& xorKey = "") {
		std::lock_guard<std::mutex> lk(_mtx);

		// 先关旧的
		if (_zf) { unzClose(_zf); _zf = nullptr; }
		_table.clear();
		_xorKeyShared.reset();
		_path.clear();
		_password.clear();

		_path = path;
		_password = password;

		// 异或 key 放堆上，生命周期跟随 _zf
		// minizip 会把 opaque 拷进它自己的堆结构，不能指向栈变量
		_xorKeyShared = std::make_shared<std::string>(xorKey);

		zlib_filefunc64_def ff;
		makeFileFunc(&ff, _xorKeyShared.get());   // opaque = string*

		_zf = unzOpen2_64(path.c_str(), &ff);
		if (!_zf || !buildTable()) {
			if (_zf) { unzClose(_zf); _zf = nullptr; }
			_xorKeyShared.reset();
			_path.clear();
			_password.clear();
			return false;
		}
		return true;
	}

	// ---------- 关闭 ----------
	void close() {
		std::lock_guard<std::mutex> lk(_mtx);
		if (_zf) { unzClose(_zf); _zf = nullptr; }
		_table.clear();
		_xorKeyShared.reset();
		_path.clear();
		_password.clear();
	}

	// ---------- 查询 ----------
	bool has(const std::string& name) const {
		std::string k = normalizeKey(name);
		std::lock_guard<std::mutex> lk(_mtx);
		return _table.find(k) != _table.end();
	}

	// ---------- 读 ----------
	bool read(const std::string& name, Data& out) const {
		std::string k = normalizeKey(name);
		std::lock_guard<std::mutex> lk(_mtx);

		if (!_zf) { out.clear(); return false; }
		auto it = _table.find(k);
		if (it == _table.end()) { out.clear(); return false; }
		return readEntry(it->second, out);
	}

	 std::string path() const {
		std::lock_guard<std::mutex> lk(_mtx);
		return _path;   //**返回 std::string 值拷贝**，
	}
private:
	// ========== 自定义 ioapi（异或在这里） ==========
	struct XorFileCtx {
		FILE* fp = nullptr;
		std::string xorKey;    // 拷一份，独立于外部
		uint64_t pos = 0;      // 当前文件位置，异或相位用
	};

	// ---- 打开 ----
	static voidpf ZCALLBACK xor_open64(voidpf opaque, const void* filename, int mode) {
		(void)mode;
		const std::string* key = (const std::string*)opaque;

		FILE* fp = FILE_OPEN64((const char*)filename, "rb");
		if (!fp) return nullptr;

		XorFileCtx* ctx = new (std::nothrow) XorFileCtx();
		if (!ctx) { fclose(fp); return nullptr; }
		ctx->fp = fp;
		ctx->xorKey = key ? *key : "";
		ctx->pos = 0;
		return ctx;
	}

	// ---- 读（异或核心） ----
	static uint32_t ZCALLBACK xor_read(voidpf, voidpf stream, void* buf, uint32_t size) {
		XorFileCtx* ctx = (XorFileCtx*)stream;
		if (!ctx || !ctx->fp) return (uint32_t)-1;

		uint32_t n = (uint32_t)fread(buf, 1, size, ctx->fp);

		if (n > 0 && !ctx->xorKey.empty()) {
			const size_t klen = ctx->xorKey.size();
			uint8_t* p = (uint8_t*)buf;
			const uint8_t* k = (const uint8_t*)ctx->xorKey.data();
			uint64_t base = ctx->pos;
			// 异或相位 = 文件偏移 % key长度
			for (uint32_t i = 0; i < n; ++i)
				p[i] ^= k[(base + i) % klen];
		}

		ctx->pos += n;
		return n;
	}

	// ---- 写（只读） ----
	static uint32_t ZCALLBACK xor_write(voidpf, voidpf, const void*, uint32_t) {
		return (uint32_t)-1;
	}

	// ---- 定位 ----
	static long ZCALLBACK xor_seek64(voidpf, voidpf stream, uint64_t offset, int origin) {
		XorFileCtx* ctx = (XorFileCtx*)stream;
		if (!ctx || !ctx->fp) return -1;

		int whence = SEEK_SET;
		if (origin == ZLIB_FILEFUNC_SEEK_CUR) whence = SEEK_CUR;
		else if (origin == ZLIB_FILEFUNC_SEEK_END) whence = SEEK_END;

		// 平台 64 位类型，避免 32 位截断
		if (FILE_SEEK64(ctx->fp, (plat_off_t)offset, whence) != 0) return -1;

		// 用实际位置回填 pos，异或相位依赖它
		int64_t newPos = (int64_t)FILE_TELL64(ctx->fp);
		if (newPos < 0) return -1;
		ctx->pos = (uint64_t)newPos;
		return 0;
	}

	// ---- 当前位置 ----
	static uint64_t ZCALLBACK xor_tell64(voidpf, voidpf stream) {
		XorFileCtx* ctx = (XorFileCtx*)stream;
		if (!ctx || !ctx->fp) return (uint64_t)-1;
		int64_t p = (int64_t)FILE_TELL64(ctx->fp);
		if (p < 0) return (uint64_t)-1;
		return (uint64_t)p;
	}

	// ---- 关闭 ----
	static int ZCALLBACK xor_close(voidpf, voidpf stream) {
		XorFileCtx* ctx = (XorFileCtx*)stream;
		if (ctx) {
			if (ctx->fp) fclose(ctx->fp);
			delete ctx;
		}
		return 0;
	}

	// ---- 错误 ----
	static int ZCALLBACK xor_error(voidpf, voidpf stream) {
		XorFileCtx* ctx = (XorFileCtx*)stream;
		if (!ctx || !ctx->fp) return -1;
		return ferror(ctx->fp);
	}

	// ---- 组装回调 ----
	static void makeFileFunc(zlib_filefunc64_def* ff, voidpf opaque) {
		memset(ff, 0, sizeof(*ff));
		ff->zopen64_file = xor_open64;
		ff->zopendisk64_file = nullptr;
		ff->zread_file = xor_read;
		ff->zwrite_file = xor_write;
		ff->ztell64_file = xor_tell64;
		ff->zseek64_file = xor_seek64;
		ff->zclose_file = xor_close;
		ff->zerror_file = xor_error;
		ff->opaque = opaque;   // string*
	}

	// ========== 建表 ==========
	// 调用时已持锁
	bool buildTable() {
		unz_global_info64 gi;
		if (unzGetGlobalInfo64(_zf, &gi) != UNZ_OK) return false;
		if (unzGoToFirstFile(_zf) != UNZ_OK) return false;

		for (uint64_t i = 0; i < gi.number_entry; ++i) {
			// 先拿文件名长度
			unz_file_info64 fiProbe;
			if (unzGetCurrentFileInfo64(_zf, &fiProbe,
				nullptr, 0, nullptr, 0, nullptr, 0) != UNZ_OK) {
				return false;
			}

			// 按实际长度分配，避免 1024 截断
			uint32_t nameLen = fiProbe.size_filename;
			uint32_t allocLen = nameLen + 1;
			if (allocLen > 65535) allocLen = 65535;   // zip 名字上限
			std::vector<char> nameBuf((size_t)allocLen, 0);

			unz_file_info64 fi;
			if (unzGetCurrentFileInfo64(_zf, &fi,
				nameBuf.data(), (uint16_t)allocLen,
				nullptr, 0, nullptr, 0) != UNZ_OK) {
				return false;
			}

			// 归一化：反斜杠→斜杠，去开头 ./
			std::string name = normalizeKey(nameBuf.data());

			// 跳过目录 entry
			if (!name.empty() && name.back() != '/') {
				Entry e;
				unzGetFilePos64(_zf, &e.pos);
				e.rawSize = fi.uncompressed_size;
				e.crc32 = fi.crc;
				e.method = fi.compression_method;
				e.flags = fi.flag;
				_table[name] = e;
			}

			if (i + 1 < gi.number_entry) {
				if (unzGoToNextFile(_zf) != UNZ_OK) return false;
			}
		}
		return true;
	}

	// ========== 读 entry（零拷贝） ==========
	// 调用时已持锁
	bool readEntry(const Entry& e, Data& out) const {
		out.clear();

		unz64_file_pos pos = e.pos;
		if (unzGoToFilePos64(_zf, &pos) != UNZ_OK) return false;

		int err = _password.empty()
			? unzOpenCurrentFile(_zf)
			: unzOpenCurrentFilePassword(_zf, _password.c_str());
		if (err != UNZ_OK) return false;

		// 空文件
		if (e.rawSize == 0) {
			unzCloseCurrentFile(_zf);
			out.fastSet(nullptr, 0);
			return true;
		}

		// 一次分配，直接写入目标内存
		unsigned char* raw = (unsigned char*)malloc((size_t)e.rawSize);
		if (!raw) { unzCloseCurrentFile(_zf); return false; }

		uint64_t totalRead = 0;
		bool readSuccess = true;
		const uint32_t CHUNK = 64 * 1024;

		while (totalRead < e.rawSize) {
			uint32_t toRead = (uint32_t)std::min<uint64_t>(CHUNK, e.rawSize - totalRead);
			int n = unzReadCurrentFile(_zf, raw + totalRead, toRead);
			if (n < 0) { readSuccess = false; break; }
			if (n == 0) break;   // 提前 EOF
			totalRead += n;
		}

		// 无论成败都 close（释放资源 + CRC 校验）
		int closeRet = unzCloseCurrentFile(_zf);

		if (!readSuccess || closeRet != UNZ_OK || totalRead != e.rawSize) {
			free(raw);
			return false;
		}

		out.fastSet(raw, totalRead);
		return true;
	}

	// ========== 成员 ==========
	std::string _path;
	std::string _password;
	std::shared_ptr<std::string> _xorKeyShared;   // 跟 _zf 同生命周期
	unzFile _zf = nullptr;
	std::unordered_map<std::string, Entry> _table;

	mutable std::mutex _mtx;   // 保护 _zf / _table
};

// ==================== 多 zip 管理器（线程安全，两阶段加锁） ====================
class ZipTableManager {
public:
	Instance(ZipTableManager);
	ZipTableManager() = default;
	~ZipTableManager() { clear(); }

	ZipTableManager(const ZipTableManager&) = delete;
	ZipTableManager& operator=(const ZipTableManager&) = delete;

	// 追加（优先级低，后查）
	bool addZip(const std::string& path,
		const std::string& password = "",
		const std::string& xorKey = "") {
		auto z = std::make_shared<ZipArchive>();
		if (!z->open(path, password, xorKey)) return false;

		std::lock_guard<std::mutex> lk(_mgrMtx);
		_archives.push_back(std::move(z));
		return true;
	}

	// 插到最前（优先级高，热更用）
	bool addZipHighPriority(const std::string& path,
		const std::string& password = "",
		const std::string& xorKey = "") {
		auto z = std::make_shared<ZipArchive>();
		if (!z->open(path, password, xorKey)) return false;

		std::lock_guard<std::mutex> lk(_mgrMtx);
		_archives.insert(_archives.begin(), std::move(z));
		return true;
	}

	// 按路径移除某个 zip
	bool removeZip(const std::string& path) {
		std::lock_guard<std::mutex> lk(_mgrMtx);
		auto it = std::remove_if(_archives.begin(), _archives.end(),
			[&](const std::shared_ptr<ZipArchive>& z) {
			return z->path() == path;
		});
		if (it == _archives.end()) return false;   // 没找到
		_archives.erase(it, _archives.end());       // 删掉
		return true;
	}

	void clear() {
		std::lock_guard<std::mutex> lk(_mgrMtx);
		_archives.clear();
	}

	bool has(const std::string& key) const {
		std::string k = normalizeKey(key);

		// 阶段 1：短暂持锁，拿目标指针
		std::shared_ptr<ZipArchive> target;
		{
			std::lock_guard<std::mutex> lk(_mgrMtx);
			for (auto& z : _archives) {
				if (z->has(k)) { target = z; break; }
			}
		}
		return target != nullptr;
	}

	// 按优先级遍历，命中就返回
	bool read(const std::string& key, Data& out) const {
		std::string k = normalizeKey(key);

		// 阶段 1：短暂持锁，拿目标指针
		// shared_ptr 保证后续 clear() 不会让 target 失效
		std::shared_ptr<ZipArchive> target;
		{
			std::lock_guard<std::mutex> lk(_mgrMtx);
			for (auto& z : _archives) {
				if (z->has(k)) { target = z; break; }
			}
		}

		// 阶段 2：不持管理器锁，只由 ZipArchive 内部锁保护
		if (!target) { out.clear(); return false; }
		return target->read(k, out);
	}

private:
	std::vector<std::shared_ptr<ZipArchive>> _archives;   // shared_ptr 保命
	mutable std::mutex _mgrMtx;                            // 保护 _archives 结构
};
OG_END
#endif