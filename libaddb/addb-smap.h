/*
Copyright 2015 Google Inc. All rights reserved.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#include "libaddb/addb.h"

#include "libcm/cm.h"
#include "libcl/cl.h"

#define ADDB_SMAP_PARTITIONS_MAX 1024

struct addb_tiled;
struct addb_tiled_pool;

typedef struct addb_smap addb_smap;

typedef unsigned long long addb_smap_id;

/**
 * @brief An SMAP table is stored as up to 1024 partitions; each
 * 	partition corresponds to a single file.
 */
typedef struct addb_smap_partition {
  /**
   * @brief The table that this partition is part of.
   */
  addb_smap* part_sm;

  /**
   * @brief Malloc'ed copy of the specific database file's name,
   *	for logging.
   */
  char* part_path;

  /**
   * @brief The tile manager for the file; shares a tile pool
   *  	with its siblings.
   *
   *  If a partition hasn't yet been opened or doesn't exist,
   *  this pointer is NULL.
   */
  struct addb_tiled* part_td;

  /**
   * @brief The virtual file size.  When appending, data is
   * 	written after this offset, and it is incremented.
   *
   *  The actual underlying file storage is allocated in
   *  page size increments.
   */
  unsigned long long part_size;

} addb_smap_partition;

/**
 * @brief Configuration parameters for a single gmap table.
 */
typedef struct addb_smap_configuration {
  /**
   * @brief How much memory to initially map for each partition of
   * this gmap */
  unsigned long long gcf_init_map;

  /**
   * @brief Lock the gmap in memory if true
   */
  bool gcf_mlock;

} addb_smap_configuration;

struct addb_smap {
  /**
  * @brief Pointer to the overall database that this map is part of.
  */
  addb_handle* sm_addb;

  /**
   * @brief Configuration data.
   */
  addb_smap_configuration sm_cf;

  /**
  * @brief Filename of the partition directory
  */
  char* sm_path;

  /**
  * @brief Basename.
  *
  * Partition filenames are generated by appending numbers to
  * sm_base at sm_base_n.
  */
  char* sm_base;

  /**
  * @brief Length of the basename, in bytes.
  */
  size_t sm_base_n;

  /**
  * @brief Index of first unoccupied partition with no higher
  *  occupied partition.
  */
  size_t sm_partition_n;

  /**
  * @brief Partitions of this SMAP; can be unoccupied.
  */
  addb_smap_partition sm_partition[1024];

  /**
  * @brief Tiled pool shared by all partitions.
  */
  struct addb_tiled_pool* sm_tiled_pool;

  /**
  * @brief The last time the SMAP index was in sync with the
  * 	istore, the istore was in this consistent state.
  *
  *  	This is the state the SMAP would go back to if it used
  * 	its backup and forgot the changes made in temporarily
  * 	allocated memory tiles overlapping file tiles.
  */
  unsigned long long sm_horizon;

  /**
  * @brief Is this SMAP backed up?
  */
  unsigned int sm_backup : 1;

  /*
  * Async context for syncing this addb_smap directory (not the files)
  */
  addb_fsync_ctx sm_dir_fsync_ctx;

  /*
  * File descriptor to the directory for use with above
  */
  int sm_dir_fd;
};

addb_smap* addb_smap_open(addb_handle* addb, char const* path, int mode,
                          unsigned long long horizon,
                          addb_smap_configuration* gcf);

int addb_smap_add(addb_smap* _sm, addb_smap_id _source, addb_smap_id _dest,
                  bool _exclusive);

int addb_smap_remove(addb_handle*, char const*);

int addb_smap_truncate(addb_smap* sm, char const* path);

int addb_smap_close(addb_smap*);

int addb_smap_status(addb_smap* _sm, cm_prefix const* _prefix,
                     addb_status_callback* _callback, void* _callback_data);

int addb_smap_status_tiles(addb_smap* _sm, cm_prefix const* _prefix,
                           addb_status_callback* _callback,
                           void* _callback_data);

int addb_smap_backup(addb_smap*, unsigned long long);

int addb_smap_checkpoint_rollback(addb_smap* _sm);

int addb_smap_checkpoint_finish_backup(addb_smap* sm, bool hard_sync,
                                       bool block);

int addb_smap_checkpoint_sync_backup(addb_smap* sm, bool hard_sync, bool block);

int addb_smap_checkpoint_sync_directory(addb_smap* sm, bool hard_sync,
                                        bool block);

int addb_smap_checkpoint_start_writes(addb_smap* sm, bool hard_sync,
                                      bool block);

int addb_smap_checkpoint_finish_writes(addb_smap* sm, bool hard_sync,
                                       bool block);

int addb_smap_checkpoint_remove_backup(addb_smap* sm, bool hard_sync,
                                       bool block);

int addb_smap_freelist_alloc(addb_smap_partition* _part, size_t _ex,
                             unsigned long long* _offset_out);

int addb_smap_freelist_free(addb_smap_partition* _part, unsigned long long _off,
                            size_t _ex);

void addb_smap_partition_initialize(addb_smap* _gm, addb_smap_partition* _part);

int addb_smap_partition_finish(addb_smap_partition*);

int addb_smap_partition_grow(addb_smap_partition*, off_t);
void addb_smap_partition_basename(addb_handle* _addb, size_t _i, char* _buf,
                                  size_t _bufsize);

int addb_smap_partition_name(addb_smap_partition* _part, size_t _i);

int addb_smap_partition_open(addb_smap_partition* _gm, int _mode);
int addb_smap_partitions_read(addb_smap*, int);

#if 0
int 		  addb_smap_partition_last(
			addb_smap_partition	* _part,
			addb_smap_id 		  _id,
			addb_smap_id		* _val_out);
#endif

int addb_smap_partition_get(addb_smap_partition* part,
                            unsigned long long offset, unsigned long long* out);

int addb_smap_partition_get_chunk(addb_smap_partition* part,
                                  unsigned long long offset_s,
                                  char const** data_s_out,
                                  char const** data_e_out,
                                  addb_tiled_reference* tref_out);

int addb_smap_partition_put(addb_smap_partition* part,
                            unsigned long long offset, unsigned long long val);

int addb_smap_partition_mem_to_file(addb_smap_partition* _part,
                                    unsigned long long _offset,
                                    char const* _source, size_t _n);

int addb_smap_partition_copy(addb_smap_partition* _part,
                             unsigned long long _destination,
                             unsigned long long _source, unsigned long long _n);

int addb_smap_partition_data(addb_smap_partition* _part, addb_smap_id _id,
                             unsigned long long* _offset_out,
                             unsigned long long* _n_out,
                             unsigned long long* _hint_out);

int addb_smap_partition_read_raw_loc(addb_smap_partition* _part,
                                     unsigned long long _offset,
                                     unsigned long long _end,
                                     unsigned char const** _ptr_out,
                                     unsigned long long* _end_out,
                                     addb_tiled_reference* _tref,
                                     char const* _file, int _line);
#define addb_smap_partition_read_raw(a, b, c, d, e, f) \
  addb_smap_partition_read_raw_loc(a, b, c, d, e, f, __FILE__, __LINE__)

addb_smap_partition* addb_smap_partition_by_id(addb_smap* _sm,
                                               addb_smap_id _id);

int addb_smap_backing(addb_smap* gm, addb_smap_id id, int* out);

unsigned long long addb_smap_horizon(addb_smap* sm);

void addb_smap_horizon_set(addb_smap* sm, unsigned long long horizon);
