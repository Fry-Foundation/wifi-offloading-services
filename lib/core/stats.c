#include "stats.h"
// #include "config.h"
#include <stdio.h>
#include <string.h>
#include <sys/statvfs.h>

DiskStats get_disk_stats(const char *path) {
    DiskStats stats = {0};

    // if (config.dev_env) {
    //     // Return typical development machine disk stats (10 GB total)
    //     stats.total_mb = 10240;     // 10 GB
    //     stats.available_mb = 8192;  // 8 GB available
    //     stats.used_mb = 2048;       // 2 GB used
    //     stats.used_percent = 20;    // 20% used
    //     return stats;
    // }

    struct statvfs stat;

    if (statvfs(path, &stat) != 0) {
        // Return zeros on error
        return stats;
    }

    // Calculate sizes in MB
    stats.total_mb = (long)((stat.f_blocks * stat.f_frsize) / (1024 * 1024));
    stats.available_mb = (long)((stat.f_bavail * stat.f_frsize) / (1024 * 1024));
    stats.used_mb = stats.total_mb - stats.available_mb;

    // Calculate used percentage
    if (stats.total_mb > 0) {
        stats.used_percent = (int)((stats.used_mb * 100) / stats.total_mb);
    }

    return stats;
}

MemoryStats get_memory_stats(void) {
    MemoryStats stats = {0};

    // if (config.dev_env) {
    //     // Return typical development machine memory stats (8 GB total)
    //     stats.total_kb = 8388608;    // 8 GB
    //     stats.free_kb = 2097152;     // 2 GB free
    //     stats.available_kb = 4194304; // 4 GB available (includes cache/buffers)
    //     stats.shared_kb = 524288;    // 512 MB shared
    //     stats.buffered_kb = 1048576; // 1 GB buffered/cached
    //     stats.used_kb = stats.total_kb - stats.free_kb;
    //     stats.used_percent = (int)((stats.used_kb * 100) / stats.total_kb);
    //     return stats;
    // }

    FILE *file = fopen("/proc/meminfo", "r");
    if (file == NULL) {
        return stats;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        unsigned long value;

        if (sscanf(line, "MemTotal: %lu kB", &value) == 1) {
            stats.total_kb = value;
        } else if (sscanf(line, "MemFree: %lu kB", &value) == 1) {
            stats.free_kb = value;
        } else if (sscanf(line, "MemAvailable: %lu kB", &value) == 1) {
            stats.available_kb = value;
        } else if (sscanf(line, "Shmem: %lu kB", &value) == 1) {
            stats.shared_kb = value;
        } else if (sscanf(line, "Buffers: %lu kB", &value) == 1) {
            stats.buffered_kb += value;
        } else if (sscanf(line, "Cached: %lu kB", &value) == 1) {
            stats.buffered_kb += value;
        }
    }

    fclose(file);

    // Calculate used memory and percentage
    stats.used_kb = stats.total_kb - stats.free_kb;
    if (stats.total_kb > 0) {
        stats.used_percent = (int)((stats.used_kb * 100) / stats.total_kb);
    }

    // If MemAvailable is not available, estimate it
    if (stats.available_kb == 0) {
        stats.available_kb = stats.free_kb + stats.buffered_kb;
    }

    return stats;
}

long get_available_disk_space_mb(const char *path) {
    DiskStats stats = get_disk_stats(path);
    return stats.available_mb;
}

long get_total_disk_space_mb(const char *path) {
    DiskStats stats = get_disk_stats(path);
    return stats.total_mb;
}

unsigned long get_total_memory_kb(void) {
    MemoryStats stats = get_memory_stats();
    return stats.total_kb;
}

unsigned long get_available_memory_kb(void) {
    MemoryStats stats = get_memory_stats();
    return stats.available_kb;
}
