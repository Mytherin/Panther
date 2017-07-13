
#include "inmemorytextfile.h"

#include "statusnotification.h"


struct FindAllInformation {
	ProjectExplorer* explorer;
	std::shared_ptr<PGStatusNotification> notification;
	TextFile* textfile;
	PGRegexHandle regex_handle;
	PGGlobSet whitelist;
	std::vector<PGFile> files;
	bool ignore_binary;
	int context_lines;
	std::shared_ptr<Task> task;
};

struct OpenFileInformation {
	std::shared_ptr<TextFile> file;
	char* base;
	lng size;
	bool delete_file;

	OpenFileInformation(std::shared_ptr<TextFile> file, char* base, lng size, bool delete_file) : file(file), base(base), size(size), delete_file(delete_file) {}
};

InMemoryTextFile::InMemoryTextFile() {

}

InMemoryTextFile::InMemoryTextFile(std::string filename) {

}

InMemoryTextFile::~InMemoryTextFile() {

}

void InMemoryTextFile::OpenFile(std::shared_ptr<TextFile> file, PGFileEncoding encoding, char* base, size_t size, bool immediate_load) {
	file->encoding = encoding;
	if (!immediate_load) {
		OpenFileInformation* info = new OpenFileInformation(file, base, size, false);
		this->current_task = std::make_shared<Task>(
			[](std::shared_ptr<Task> task, void* inp) {
			OpenFileInformation* info = (OpenFileInformation*)inp;
			info->file->OpenFile(info->base, info->size, info->delete_file);
			delete info;
		}
		, info);
		Scheduler::RegisterTask(this->current_task, PGTaskUrgent);
	} else {
		OpenFile(base, size, false);
	}
}

void InMemoryTextFile::ReadFile(std::shared_ptr<TextFile> file, bool immediate_load, bool ignore_binary) {
	if (!immediate_load) {
		OpenFileInformation* info = new OpenFileInformation(file, nullptr, 0, ignore_binary);
		this->current_task = std::make_shared<Task>(
			[](std::shared_ptr<Task> task, void* inp) {
			OpenFileInformation* info = (OpenFileInformation*)inp;
			info->file->ActuallyReadFile(info->file, info->delete_file);
			delete info;
		}
		, info);
		Scheduler::RegisterTask(this->current_task, PGTaskUrgent);
	} else {
		ActuallyReadFile(file, ignore_binary);
	}
}
