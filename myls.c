#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

#define THRESHOLD 15
#define SECONDS_IN_30_DAYS (30 * 24 * 60 * 60)

#define RED_COLOR   "\033[1;31m"
#define RESET_COLOR "\033[0m"

bool show_size = false;

bool is_file_important(const char *path) {
	struct stat file_stat;
	if (stat(path, &file_stat) != 0) return false;

	time_t current_time = time(NULL);
	double seconds_since_last_access = difftime(current_time, file_stat.st_atime);
	if (seconds_since_last_access > SECONDS_IN_30_DAYS) return false;

	char attr_val[16] = {0};
	if (getxattr(path, "user.use_count", attr_val, sizeof(attr_val) - 1) > 0) {
		if (atoi(attr_val) > THRESHOLD) {
			return true;
		}
	}
	return false;
}

bool is_dir_important(const char *dir_path) {
	DIR *dir = opendir(dir_path);
	if (!dir) return false;

	struct dirent *entry;
	bool important_found = false;

	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
			continue;
		}

		char full_path[1024];
		snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

		struct stat entry_stat;
		if (stat(full_path, &entry_stat) == 0) {
			if (S_ISDIR(entry_stat.st_mode)) {
				if (is_dir_important(full_path)) {
					important_found = true;
					break;
				}
			} else if (S_ISREG(entry_stat.st_mode)) {
				if (is_file_important(full_path)) {
					important_found = true;
					break;
				}
			}
		}
	}
	closedir(dir);
	return important_found;
}

void process_entry(const char *filename) {
	struct stat file_stat;
	if (stat(filename, &file_stat) != 0) return;

	bool is_important = false;

	if (S_ISDIR(file_stat.st_mode)) {
		is_important = is_dir_important(filename);
	} else {
		is_important = is_file_important(filename);
	}

	char size_str[32] = "";
	if (show_size) {
		snprintf(size_str, sizeof(size_str), "%5ld ", file_stat.st_size);
	}

	if (is_important) {
		printf("%s%s[★] %s%s ", RED_COLOR, size_str, filename, RESET_COLOR);
	} else {
		printf("    %s%s ", size_str, filename);
	}
}

int main(int argc, char *argv[]) {
	int opt;

	while ((opt = getopt(argc, argv, "sh")) != -1) {
		switch (opt) {
			case 's':
				show_size = true;
				break;

			case 'h':
				printf("Użycie: %s [-s] [-h]\n", argv[0]);
				printf("  -s	show file size\n");
				printf("  -h	show help\n");
				return 0;

			default:
				fprintf(stderr, "Usage: %s [-s] [-h]\n", argv[0]);
				return 1;
		}
	}

	DIR *dir = opendir(".");
	if (dir == NULL) {
		perror("Can't open this file");
		return 1;
	}

	int count = 0;
	int items_per_rows = 10;

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
			continue;
		}

		process_entry(entry->d_name);
		count++;

		if (count % items_per_rows == 0) {
			printf("\n");
		}
	}

	if (count % items_per_rows != 0) {
		printf("\n");
	}

	closedir(dir);
	return 0;
}
