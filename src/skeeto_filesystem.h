#pragma once
// Minimal L4D2 IFileSystem layout (Necola-compatible) for style JSON discovery.
// Do not include Source SDK; vtable order must match VFileSystem018.

enum InitReturnVal_t {
	INIT_FAILED = 0,
	INIT_OK,
	INIT_LAST_VAL,
};

enum FileSystemSeek_t {
	FILESYSTEM_SEEK_HEAD = 0,
	FILESYSTEM_SEEK_CURRENT = 1,
	FILESYSTEM_SEEK_TAIL = 2,
};

enum FilesystemMountRetval_t {
	FILESYSTEM_MOUNT_OK = 0,
	FILESYSTEM_MOUNT_FAILED,
};

enum SearchPathAdd_t {
	PATH_ADD_TO_HEAD,
	PATH_ADD_TO_TAIL,
	PATH_ADD_TO_TAIL_ATINDEX,
};

enum PathTypeFilter_t {
	FILTER_NONE = 0,
	FILTER_CULLPACK = 1,
	FILTER_CULLNONPACK = 2,
	FILTER_CULLLOCALIZED = 3,
	FILTER_CULLLOCALIZED_ANY = 4,
};

using FileHandle_t = void*;
using FileFindHandle_t = int;
using PathTypeQuery_t = unsigned int;
using FSAllocFunc_t = void* (*)(const char*, unsigned);

#define SKEETO_FS_INVALID_HANDLE ((FileHandle_t)0)

class IAppSystem {
public:
	virtual bool Connect(void* factory) = 0;
	virtual void Disconnect() = 0;
	virtual void* QueryInterface(const char* pInterfaceName) = 0;
	virtual InitReturnVal_t Init() = 0;
	virtual void Shutdown() = 0;
};

class IBaseFileSystem {
public:
	virtual int Read(void* pOutput, int size, FileHandle_t file) = 0;
	virtual int Write(void const* pInput, int size, FileHandle_t file) = 0;
	virtual FileHandle_t Open(const char* pFileName, const char* pOptions, const char* pathID = nullptr) = 0;
	virtual void Close(FileHandle_t file) = 0;
	virtual void Seek(FileHandle_t file, int pos, FileSystemSeek_t seekType) = 0;
	virtual unsigned int Tell(FileHandle_t file) = 0;
	virtual unsigned int Size(FileHandle_t file) = 0;
	virtual unsigned int Size(const char* pFileName, const char* pPathID = nullptr) = 0;
	virtual void Flush(FileHandle_t file) = 0;
	virtual bool Precache(const char* pFileName, const char* pPathID = nullptr) = 0;
	virtual bool FileExists(const char* pFileName, const char* pPathID = nullptr) = 0;
	virtual bool IsFileWritable(char const* pFileName, const char* pPathID = nullptr) = 0;
	virtual bool SetFileWritable(char const* pFileName, bool writable, const char* pPathID = nullptr) = 0;
	virtual long GetFileTime(const char* pFileName, const char* pPathID = nullptr) = 0;
	virtual bool ReadFile(const char* pFileName, const char* pPath, void* buf, int nMaxBytes = 0, int nStartingByte = 0, FSAllocFunc_t pfnAlloc = nullptr) = 0;
	virtual bool WriteFile(const char* pFileName, const char* pPath, void* buf) = 0;
	virtual bool UnzipFile(const char* pFileName, const char* pPath, const char* pDestination) = 0;
};

class IFileSystem : public IAppSystem, public IBaseFileSystem {
public:
	virtual bool IsSteam() const = 0;
	virtual FilesystemMountRetval_t MountSteamContent(int nExtraAppId = -1) = 0;
	virtual void AddSearchPath(const char* pPath, const char* pathID, SearchPathAdd_t addType = PATH_ADD_TO_TAIL) = 0;
	virtual bool RemoveSearchPath(const char* pPath, const char* pathID = nullptr) = 0;
	virtual void RemoveAllSearchPaths(void) = 0;
	virtual void RemoveSearchPaths(const char* szPathID) = 0;
	virtual void MarkPathIDByRequestOnly(const char* pPathID, bool bRequestOnly) = 0;
	virtual const char* RelativePathToFullPath(const char* pFileName, const char* pPathID, char* pLocalPath, int localPathBufferSize, PathTypeFilter_t pathFilter = FILTER_NONE, PathTypeQuery_t* pPathType = nullptr) = 0;
	virtual int GetSearchPath(const char* pathID, bool bGetPackFiles, char* pPath, int nMaxLen) = 0;
	virtual bool AddPackFile(const char* fullpath, const char* pathID) = 0;
	virtual bool IsLocalizedPath(const char*) = 0;
	virtual void RemoveFile(char const* pRelativePath, const char* pathID = nullptr) = 0;
	virtual bool RenameFile(char const* pOldPath, char const* pNewPath, const char* pathID = nullptr) = 0;
	virtual void CreateDirHierarchy(const char* path, const char* pathID = nullptr) = 0;
	virtual bool IsDirectory(const char* pFileName, const char* pathID = nullptr) = 0;
	virtual void FileTimeToString(char* pStrip, int maxCharsIncludingTerminator, long fileTime) = 0;
	virtual void SetBufferSize(FileHandle_t file, unsigned nBytes) = 0;
	virtual bool IsOk(FileHandle_t file) = 0;
	virtual bool EndOfFile(FileHandle_t file) = 0;
	virtual char* ReadLine(char* pOutput, int maxChars, FileHandle_t file) = 0;
	virtual int FPrintf(FileHandle_t file, const char* pFormat, ...) = 0;
	virtual void* LoadModule(const char* pFileName, const char* pPathID = nullptr, bool bValidatedDllOnly = true) = 0;
	virtual void UnloadModule(void* pModule) = 0;
	virtual const char* FindFirst(const char* pWildCard, FileFindHandle_t* pHandle) = 0;
	virtual const char* FindNext(FileFindHandle_t handle) = 0;
	virtual bool FindIsDirectory(FileFindHandle_t handle) = 0;
	virtual void FindClose(FileFindHandle_t handle) = 0;
	virtual const char* FindFirstEx(const char* pWildCard, const char* pPathID, FileFindHandle_t* pHandle) = 0;
};
