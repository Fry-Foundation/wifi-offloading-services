#ifndef STATS_H
#define STATS_H

// Disk statistics (in MB)
typedef struct {
    long total_mb;
    long available_mb;
    long used_mb;
    int used_percent;
} DiskStats;

// Memory statistics (in KB)
typedef struct {
    unsigned long total_kb;
    unsigned long available_kb;  
    unsigned long used_kb;
    unsigned long free_kb;
    unsigned long shared_kb;
    unsigned long buffered_kb;
    int used_percent;
} MemoryStats;

// Function declarations

/**
 * Get disk statistics for the specified path
 * @param path The filesystem path to get stats for
 * @return DiskStats structure with disk information, or zeros on error
 */
DiskStats get_disk_stats(const char *path);

/**
 * Get memory statistics from /proc/meminfo
 * @return MemoryStats structure with memory information, or zeros on error  
 */
MemoryStats get_memory_stats(void);

/**
 * Get available disk space in MB for the specified path
 * Convenience function that returns just the available space
 * @param path The filesystem path to check
 * @return Available disk space in MB, or -1 on error
 */
long get_available_disk_space_mb(const char *path);

/**
 * Get total disk space in MB for the specified path
 * Convenience function that returns just the total space
 * @param path The filesystem path to check
 * @return Total disk space in MB, or -1 on error
 */
long get_total_disk_space_mb(const char *path);

/**
 * Get total memory in KB
 * Convenience function that returns just the total memory
 * @return Total memory in KB, or -1 on error
 */
unsigned long get_total_memory_kb(void);

/**
 * Get available memory in KB  
 * Convenience function that returns just the available memory
 * @return Available memory in KB, or -1 on error
 */
unsigned long get_available_memory_kb(void);

#endif // STATS_H 