
#include "ZipUtils.h"

#ifdef MINIZIP_FROM_SYSTEM
#include <minizip/unzip.h>
#else // from our embedded sources
#include "unzip.h"
#endif
#include "ioapi_mem.h"
#include <memory>

#include <zlib.h>
#include <assert.h>
#include <stdlib.h>
#include <set>
 
#include "OGData.h"

#include <map>
#include <unordered_map>
 


#define OG_HOST_IS_BIG_ENDIAN (bool)(*(unsigned short *)"\0\xff" < 0x100) 
#define OG_SWAP32(i)  ((i & 0x000000ff) << 24 | (i & 0x0000ff00) << 8 | (i & 0x00ff0000) >> 8 | (i & 0xff000000) >> 24)
#define OG_SWAP16(i)  ((i & 0x00ff) << 8 | (i &0xff00) >> 8)   
#define OG_SWAP_INT32_LITTLE_TO_HOST(i) ((OG_HOST_IS_BIG_ENDIAN == true)? OG_SWAP32(i) : (i) )
#define OG_SWAP_INT16_LITTLE_TO_HOST(i) ((OG_HOST_IS_BIG_ENDIAN == true)? OG_SWAP16(i) : (i) )
#define OG_SWAP_INT32_BIG_TO_HOST(i)    ((OG_HOST_IS_BIG_ENDIAN == true)? (i) : OG_SWAP32(i) )
#define OG_SWAP_INT16_BIG_TO_HOST(i)    ((OG_HOST_IS_BIG_ENDIAN == true)? (i):  OG_SWAP16(i) )

OG_BEGIN
// minizip 1.2.0 is same with other platforms
#define unzGoToFirstFile64(A,B,C,D) unzGoToFirstFile2(A,B,C,D, NULL, 0, NULL, 0)
#define unzGoToNextFile64(A,B,C,D) unzGoToNextFile2(A,B,C,D, NULL, 0, NULL, 0)



unsigned int ZipUtils::s_uEncryptedPvrKeyParts[4] = {0,0,0,0};
unsigned int ZipUtils::s_uEncryptionKey[1024];
bool ZipUtils::s_bEncryptionKeyIsValid = false;

// --------------------- ZipUtils ---------------------

inline void ZipUtils::decodeEncodedPvr(unsigned int *data, ssize_t len)
{
    const int enclen = 1024;
    const int securelen = 512;
    const int distance = 64;
    
    // check if key was set
    // make sure to call caw_setkey_part() for all 4 key parts
    OGASSERT(s_uEncryptedPvrKeyParts[0] != 0, "orange: OGZ file is encrypted but key part 0 is not set. Did you call ZipUtils::setPvrEncryptionKeyPart(...)?");
    OGASSERT(s_uEncryptedPvrKeyParts[1] != 0, "orange: OGZ file is encrypted but key part 1 is not set. Did you call ZipUtils::setPvrEncryptionKeyPart(...)?");
    OGASSERT(s_uEncryptedPvrKeyParts[2] != 0, "orange: OGZ file is encrypted but key part 2 is not set. Did you call ZipUtils::setPvrEncryptionKeyPart(...)?");
    OGASSERT(s_uEncryptedPvrKeyParts[3] != 0, "orange: OGZ file is encrypted but key part 3 is not set. Did you call ZipUtils::setPvrEncryptionKeyPart(...)?");
    
    // create long key
    if(!s_bEncryptionKeyIsValid)
    {
        unsigned int y, p, e;
        unsigned int rounds = 6;
        unsigned int sum = 0;
        unsigned int z = s_uEncryptionKey[enclen-1];
        
        do
        {
#define DELTA 0x9e3779b9
#define MX (((z>>5^y<<2) + (y>>3^z<<4)) ^ ((sum^y) + (s_uEncryptedPvrKeyParts[(p&3)^e] ^ z)))
            
            sum += DELTA;
            e = (sum >> 2) & 3;
            
            for (p = 0; p < enclen - 1; p++)
            {
                y = s_uEncryptionKey[p + 1];
                z = s_uEncryptionKey[p] += MX;
            }
            
            y = s_uEncryptionKey[0];
            z = s_uEncryptionKey[enclen - 1] += MX;
            
        } while (--rounds);
        
        s_bEncryptionKeyIsValid = true;
    }
    
    int b = 0;
    int i = 0;
    
    // encrypt first part completely
    for(; i < len && i < securelen; i++)
    {
        data[i] ^= s_uEncryptionKey[b++];
        
        if(b >= enclen)
        {
            b = 0;
        }
    }
    
    // encrypt second section partially
    for(; i < len; i += distance)
    {
        data[i] ^= s_uEncryptionKey[b++];
        
        if(b >= enclen)
        {
            b = 0;
        }
    }
}

inline unsigned int ZipUtils::checksumPvr(const unsigned int *data, ssize_t len)
{
    unsigned int cs = 0;
    const int cslen = 128;
    
    len = (len < cslen) ? len : cslen;
    
    for(int i = 0; i < len; i++)
    {
        cs = cs ^ data[i];
    }
    
    return cs;
}

// memory in iPhone is precious
// Should buffer factor be 1.5 instead of 2 ?
#define BUFFER_INC_FACTOR (2)

int ZipUtils::inflateMemoryWithHint(unsigned char *in, ssize_t inLength, unsigned char **out, ssize_t *outLength, ssize_t outLengthHint)
{
    /* ret value */
    int err = Z_OK;
    
    ssize_t bufferSize = outLengthHint;
    *out = (unsigned char*)malloc(bufferSize);
    
    z_stream d_stream; /* decompression stream */
    d_stream.zalloc = (alloc_func)0;
    d_stream.zfree = (free_func)0;
    d_stream.opaque = (voidpf)0;
    
    d_stream.next_in  = in;
    d_stream.avail_in = static_cast<unsigned int>(inLength);
    d_stream.next_out = *out;
    d_stream.avail_out = static_cast<unsigned int>(bufferSize);
    
    /* window size to hold 256k */
    if( (err = inflateInit2(&d_stream, 15 + 32)) != Z_OK )
        return err;
    
    for (;;)
    {
        err = inflate(&d_stream, Z_NO_FLUSH);
        
        if (err == Z_STREAM_END)
        {
            break;
        }
        
        switch (err)
        {
            case Z_NEED_DICT:
                err = Z_DATA_ERROR;
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                inflateEnd(&d_stream);
                return err;
        }
        
        // not enough memory ?
        if (err != Z_STREAM_END)
        {
            *out = (unsigned char*)realloc(*out, bufferSize * BUFFER_INC_FACTOR);
            
            /* not enough memory, ouch */
            if (! *out )
            {
                OGLOG("orange: ZipUtils: realloc failed");
                inflateEnd(&d_stream);
                return Z_MEM_ERROR;
            }
            
            d_stream.next_out = *out + bufferSize;
            d_stream.avail_out = static_cast<unsigned int>(bufferSize);
            bufferSize *= BUFFER_INC_FACTOR;
        }
    }
    
    *outLength = bufferSize - d_stream.avail_out;
    err = inflateEnd(&d_stream);
    return err;
}

ssize_t ZipUtils::inflateMemoryWithHint(unsigned char *in, ssize_t inLength, unsigned char **out, ssize_t outLengthHint)
{
    ssize_t outLength = 0;
    int err = inflateMemoryWithHint(in, inLength, out, &outLength, outLengthHint);
    
    if (err != Z_OK || *out == nullptr) {
        if (err == Z_MEM_ERROR)
        {
            OGLOG("orange: ZipUtils: Out of memory while decompressing map data!");
        } else
            if (err == Z_VERSION_ERROR)
            {
                OGLOG("orange: ZipUtils: Incompatible zlib version!");
            } else
                if (err == Z_DATA_ERROR)
                {
                    OGLOG("orange: ZipUtils: Incorrect zlib compressed data!");
                }
                else
                {
                    OGLOG("orange: ZipUtils: Unknown error while decompressing map data!");
                }

        if(*out) {
            free(*out);
            *out = nullptr;
        }
        outLength = 0;
    }
    
    return outLength;
}

ssize_t ZipUtils::inflateMemory(unsigned char *in, ssize_t inLength, unsigned char **out)
{
    // 256k for hint
    return inflateMemoryWithHint(in, inLength, out, 256 * 1024);
}

int ZipUtils::inflateGZipFile(const char *path, unsigned char **out)
{
    int len;
    unsigned int offset = 0;
    
    OGASSERT(out, "out can't be nullptr.");
    OGASSERT(&*out, "&*out can't be nullptr.");
    
    gzFile inFile = gzopen(FileUtils::getInstance()->getSuitableFOpen(path).c_str(), "rb");
    if( inFile == nullptr ) {
        OGLOG("orange: ZipUtils: error open gzip file: %s", path);
        return -1;
    }
    
    /* 512k initial decompress buffer */
    unsigned int bufferSize = 512 * 1024;
    unsigned int totalBufferSize = bufferSize;
    
    *out = (unsigned char*)malloc( bufferSize );
    if(*out == NULL)
    {
        OGLOG("orange: ZipUtils: out of memory");
        return -1;
    }
    
    for (;;) {
        len = gzread(inFile, *out + offset, bufferSize);
        if (len < 0)
        {
            OGLOG("orange: ZipUtils: error in gzread");
            free( *out );
            *out = nullptr;
            return -1;
        }
        if (len == 0)
        {
            break;
        }
        
        offset += len;
        
        // finish reading the file
        if( (unsigned int)len < bufferSize )
        {
            break;
        }
        
        bufferSize *= BUFFER_INC_FACTOR;
        totalBufferSize += bufferSize;
        unsigned char *tmp = (unsigned char*)realloc(*out, totalBufferSize );
        
        if( ! tmp )
        {
            OGLOG("orange: ZipUtils: out of memory");
            free( *out );
            *out = nullptr;
            return -1;
        }
        
        *out = tmp;
    }
    
    if (gzclose(inFile) != Z_OK)
    {
        OGLOG("orange: ZipUtils: gzclose failed");
    }
    
    return offset;
}

bool ZipUtils::isOGZFile(const char *path)
{
    // load file into memory
    Data compressedData = FileUtils::getInstance()->getDataFromFile(path);

    if (compressedData.isNull())
    {
        OGLOG("orange: ZipUtils: loading file failed");
        return false;
    }

    return isOGZBuffer(compressedData.getBytes(), compressedData.getSize());
}

bool ZipUtils::isOGZBuffer(const unsigned char *buffer, ssize_t len)
{
    if (static_cast<size_t>(len) < sizeof(struct OGZHeader))
    {
        return false;
    }
    
    struct OGZHeader *header = (struct OGZHeader*) buffer;
    return header->sig[0] == 'C' && header->sig[1] == 'C' && header->sig[2] == 'Z' && (header->sig[3] == '!' || header->sig[3] == 'p');
}


bool ZipUtils::isGZipFile(const char *path)
{
    // load file into memory
    Data compressedData = FileUtils::getInstance()->getDataFromFile(path);

    if (compressedData.isNull())
    {
        OGLOG("orange: ZipUtils: loading file failed");
        return false;
    }

    return isGZipBuffer(compressedData.getBytes(), compressedData.getSize());
}

bool ZipUtils::isGZipBuffer(const unsigned char *buffer, ssize_t len)
{
    if (len < 2)
    {
        return false;
    }

    return buffer[0] == 0x1F && buffer[1] == 0x8B;
}


int ZipUtils::inflateOGZBuffer(const unsigned char *buffer, ssize_t bufferLen, unsigned char **out)
{
    struct OGZHeader *header = (struct OGZHeader*) buffer;

    // verify header
    if( header->sig[0] == 'C' && header->sig[1] == 'C' && header->sig[2] == 'Z' && header->sig[3] == '!' )
    {
        // verify header version
         unsigned int version = OG_SWAP_INT16_BIG_TO_HOST( header->version );
         if( version > 2 )
         {
             OGLOG("orange: Unsupported OGZ header format");
             return -1;
         }

        // verify compression format
        if(OG_SWAP_INT16_BIG_TO_HOST(header->compression_type) != OGZ_COMPRESSION_ZLIB )
        {
            OGLOG("orange: OGZ Unsupported compression method");
             return -1;
        }
    }
    else if( header->sig[0] == 'C' && header->sig[1] == 'C' && header->sig[2] == 'Z' && header->sig[3] == 'p' )
    {
        // encrypted ogz file
        header = (struct OGZHeader*) buffer;

        // verify header version
        unsigned int version = OG_SWAP_INT16_BIG_TO_HOST( header->version );
        if( version > 0 )
        {
            OGLOG("orange: Unsupported OGZ header format");
            return -1;
        }

        // verify compression format
        if(OG_SWAP_INT16_BIG_TO_HOST(header->compression_type) != OGZ_COMPRESSION_ZLIB )
        {
            OGLOG("orange: OGZ Unsupported compression method");
            return -1;
        }

        // decrypt
        unsigned int* ints = (unsigned int*)(buffer+12);
        ssize_t enclen = (bufferLen-12)/4;

        decodeEncodedPvr(ints, enclen);

#if ORANGE_DEBUG > 0
        // verify checksum in debug mode
        unsigned int calculated = checksumPvr(ints, enclen);
        unsigned int required = OG_SWAP_INT32_BIG_TO_HOST( header->reserved );

        if(calculated != required)
        {
            OGLOG("orange: Can't decrypt image file. Is the decryption key valid?");
            return -1;
        }
#endif
    }
    else
    {
        OGLOG("orange: Invalid OGZ file");
        return -1;
    }

    unsigned int len =OG_SWAP_INT32_BIG_TO_HOST( header->len );

    *out = (unsigned char*)malloc( len );
    if(! *out )
    {
        OGLOG("orange: OGZ: Failed to allocate memory for texture");
        return -1;
    }

    unsigned long destlen = len;
    size_t source = (size_t) buffer + sizeof(*header);
    int ret = uncompress(*out, &destlen, (Bytef*)source, bufferLen - sizeof(*header) );

    if( ret != Z_OK )
    {
        OGLOG("orange: OGZ: Failed to uncompress data");
        free( *out );
        *out = nullptr;
        return -1;
    }

    return len;
}

int ZipUtils::inflateOGZFile(const char *path, unsigned char **out)
{
    OGASSERT(out, "Invalid pointer for buffer!");
    
    // load file into memory
    Data compressedData = FileUtils::getInstance()->getDataFromFile(path);
    
    if (compressedData.isNull())
    {
        OGLOG("orange: Error loading OGZ compressed file");
        return -1;
    }
    
    return inflateOGZBuffer(compressedData.getBytes(), compressedData.getSize(), out);
}

void ZipUtils::setPvrEncryptionKeyPart(int index, unsigned int value)
{
    OGASSERT(index >= 0, "orange: key part index cannot be less than 0");
    OGASSERT(index <= 3, "orange: key part index cannot be greater than 3");
    
    if(s_uEncryptedPvrKeyParts[index] != value)
    {
        s_uEncryptedPvrKeyParts[index] = value;
        s_bEncryptionKeyIsValid = false;
    }
}

void ZipUtils::setPvrEncryptionKey(unsigned int keyPart1, unsigned int keyPart2, unsigned int keyPart3, unsigned int keyPart4)
{
    setPvrEncryptionKeyPart(0, keyPart1);
    setPvrEncryptionKeyPart(1, keyPart2);
    setPvrEncryptionKeyPart(2, keyPart3);
    setPvrEncryptionKeyPart(3, keyPart4);
}

// --------------------- ZipFile ---------------------
// from unzip.cpp
#define UNZ_MAXFILENAMEINZIP 256

static const std::string emptyFilename("");

struct ZipEntryInfo
{
    unz_file_pos pos;
    uLong uncompressed_size;
};

class ZipFilePrivate
{
public:
    unzFile zipFile;
    std::unique_ptr<ourmemory_s> memfs;
    
    // std::unordered_map is faster if available on the platform
    typedef std::unordered_map<std::string, struct ZipEntryInfo> FileListContainer;
    FileListContainer fileList;
};

ZipFile *ZipFile::createWithBuffer(const void* buffer, uLong size)
{
    ZipFile *zip = new (std::nothrow) ZipFile();
    if (zip && zip->initWithBuffer(buffer, size)) {
        return zip;
    } else {
        delete zip;
        return nullptr;
    }
}

ZipFile::ZipFile()
: _data(new ZipFilePrivate)
{
    _data->zipFile = nullptr;
}

ZipFile::ZipFile(const std::string &zipFile, const std::string &filter)
: _data(new ZipFilePrivate)
{
    _data->zipFile = unzOpen(FileUtils::getInstance()->getSuitableFOpen(zipFile).c_str());
    setFilter(filter);
}

ZipFile::~ZipFile()
{
    if (_data && _data->zipFile)
    {
        unzClose(_data->zipFile);
    }

    OG_SAFE_DELETE(_data);
}

bool ZipFile::setFilter(const std::string &filter)
{
    bool ret = false;
    do
    {
      //  OG_BREAK_IF(!_data);
      //  OG_BREAK_IF(!_data->zipFile);
        
        // clear existing file list
        _data->fileList.clear();
        
        // UNZ_MAXFILENAMEINZIP + 1 - it is done so in unzLocateFile
        char szCurrentFileName[UNZ_MAXFILENAMEINZIP + 1];
        unz_file_info64 fileInfo;
        
        // go through all files and store position information about the required files
        int err = unzGoToFirstFile64(_data->zipFile, &fileInfo,
                                     szCurrentFileName, sizeof(szCurrentFileName) - 1);
        while (err == UNZ_OK)
        {
            unz_file_pos posInfo;
            int posErr = unzGetFilePos(_data->zipFile, &posInfo);
            if (posErr == UNZ_OK)
            {
                std::string currentFileName = szCurrentFileName;
                // cache info about filtered files only (like 'assets/')
                if (filter.empty()
                    || currentFileName.substr(0, filter.length()) == filter)
                {
                    ZipEntryInfo entry;
                    entry.pos = posInfo;
                    entry.uncompressed_size = (uLong)fileInfo.uncompressed_size;
                    _data->fileList[currentFileName] = entry;
                }
            }
            // next file - also get the information about it
            err = unzGoToNextFile64(_data->zipFile, &fileInfo,
                                    szCurrentFileName, sizeof(szCurrentFileName) - 1);
        }
        ret = true;
        
    } while(false);
    
    return ret;
}

bool ZipFile::fileExists(const std::string &fileName) const
{
    bool ret = false;
    do
    {
       // OG_BREAK_IF(!_data);
        
        ret = _data->fileList.find(fileName) != _data->fileList.end();
    } while(false);
    
    return ret;
}

std::vector<std::string> ZipFile::listFiles(const std::string &pathname) const
{

    // filter files which `filename.startsWith(pathname)`
    // then make each path unique

    std::set<std::string> fileSet;
    ZipFilePrivate::FileListContainer::const_iterator it = _data->fileList.begin();
    ZipFilePrivate::FileListContainer::const_iterator end = _data->fileList.end();
    //ensure pathname ends with `/` as a directory
    std::string dirname = pathname[pathname.length() -1] == '/' ? pathname : pathname + "/";
    while(it != end)
    {
        const std::string &filename = it->first;
        if(filename.substr(0, dirname.length()) == dirname)
        {
            std::string suffix = filename.substr(dirname.length());
            auto pos = suffix.find('/');
            if (pos == std::string::npos)
            {
                fileSet.insert(suffix);
            }
            else {
                //fileSet.insert(parts[0] + "/");
                fileSet.insert(suffix.substr(0, pos + 1));
            }
        }
        it++;
    }

    return std::vector<std::string>(fileSet.begin(), fileSet.end());
}

unsigned char *ZipFile::getFileData(const std::string &fileName, ssize_t *size)
{
    unsigned char * buffer = nullptr;
    if (size)
        *size = 0;

    do
    {
       // OG_BREAK_IF(!_data->zipFile);
       // OG_BREAK_IF(fileName.empty());
        
        ZipFilePrivate::FileListContainer::const_iterator it = _data->fileList.find(fileName);
      //  OG_BREAK_IF(it ==  _data->fileList.end());
        
        ZipEntryInfo fileInfo = it->second;
        
        int nRet = unzGoToFilePos(_data->zipFile, &fileInfo.pos);
       // OG_BREAK_IF(UNZ_OK != nRet);
        
        nRet = unzOpenCurrentFile(_data->zipFile);
       // OG_BREAK_IF(UNZ_OK != nRet);
        
        buffer = (unsigned char*)malloc(fileInfo.uncompressed_size);
        int   nSize = unzReadCurrentFile(_data->zipFile, buffer, static_cast<unsigned int>(fileInfo.uncompressed_size));
       OGASSERT(nSize == 0 || nSize == (int)fileInfo.uncompressed_size, "the file size is wrong");
        
        if (size)
        {
            *size = fileInfo.uncompressed_size;
        }
        unzCloseCurrentFile(_data->zipFile);
    } while (0);
    
    return buffer;
}

bool ZipFile::getFileData(const std::string &fileName, ResizableBuffer* buffer)
{
    bool res = false;
    do
    {
//         OG_BREAK_IF(!_data->zipFile);
//         OG_BREAK_IF(fileName.empty());
        
        ZipFilePrivate::FileListContainer::const_iterator it = _data->fileList.find(fileName);
       // OG_BREAK_IF(it ==  _data->fileList.end());
        
        ZipEntryInfo fileInfo = it->second;
        
        int nRet = unzGoToFilePos(_data->zipFile, &fileInfo.pos);
       // OG_BREAK_IF(UNZ_OK != nRet);
        
        nRet = unzOpenCurrentFile(_data->zipFile);
       // OG_BREAK_IF(UNZ_OK != nRet);
        
        buffer->resize(fileInfo.uncompressed_size);
        int   nSize = unzReadCurrentFile(_data->zipFile, buffer->buffer(), static_cast<unsigned int>(fileInfo.uncompressed_size));
        OGASSERT(nSize == 0 || nSize == (int)fileInfo.uncompressed_size, "the file size is wrong");
        unzCloseCurrentFile(_data->zipFile);
        res = true;
    } while (0);
    
    return res;
}

std::string ZipFile::getFirstFilename()
{
    if (unzGoToFirstFile(_data->zipFile) != UNZ_OK) return emptyFilename;
    std::string path;
    unz_file_info info;
    getCurrentFileInfo(&path, &info);
    return path;
}

std::string ZipFile::getNextFilename()
{
    if (unzGoToNextFile(_data->zipFile) != UNZ_OK) return emptyFilename;
    std::string path;
    unz_file_info info;
    getCurrentFileInfo(&path, &info);
    return path;
}

int ZipFile::getCurrentFileInfo(std::string *filename, unz_file_info *info)
{
    char path[FILENAME_MAX + 1];
    int ret = unzGetCurrentFileInfo(_data->zipFile, info, path, sizeof(path), nullptr, 0, nullptr, 0);
    if (ret != UNZ_OK) {
        *filename = emptyFilename;
    } else {
        filename->assign(path);
    }
    return ret;
}

bool ZipFile::initWithBuffer(const void *buffer, uLong size)
{
    if (!buffer || size == 0) return false;

    zlib_filefunc_def memory_file = { 0 };
    
    std::unique_ptr<ourmemory_t> memfs(new(std::nothrow) ourmemory_t{ (char*)const_cast<void*>(buffer), static_cast<uint32_t>(size), 0, 0, 0 });
    if (!memfs) return false;
    fill_memory_filefunc(&memory_file, memfs.get());
    
    _data->zipFile = unzOpen2(nullptr, &memory_file);
    if (!_data->zipFile) return false;
    _data->memfs = std::move(memfs);

    setFilter(emptyFilename);
    return true;
}

OG_END
