/*
 * Copyright (c) 2001, Robert Collins.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 *
 *     A copy of the GNU General Public License can be found at
 *     http://www.gnu.org/
 *
 * Written by Robert Collins  <rbtcollins@hotmail.com>
 *
 */

#ifndef _IO_STREAM_H_
#define _IO_STREAM_H_

/* this is the parent class for all IO operations. It's flexable enough to be cover for
 * HTTP access, local file access, and files being extracted from archives.
 * It also encapsulates the idea of an archive, and all non-archives become the special 
 * case.
 */

/* Some things don't fit cleanly just - TODO
 * make mkdir_p fit in the hierarchy
 */

//Where is this defined?
typedef signed long ssize_t;

#if __GNUC__
#define _ATTR_(foo) __attribute__ foo
#else
#define _ATTR_(foo)
#endif

typedef enum
{
  PATH_TO_DIR,
  PATH_TO_FILE
}
path_type_t;

typedef enum
{
  IO_STREAM_INVALID,
  IO_STREAM_STREAM,
  IO_STREAM_COMPRESS,
  IO_STREAM_ARCHIVE
}
io_stream_type_t;

typedef enum
{
  IO_STREAM_SYMLINK,
  IO_STREAM_HARDLINK
}
io_stream_link_t;

typedef enum
{
  IO_SEEK_SET = SEEK_SET,
  IO_SEEK_END = SEEK_END,
  IO_SEEK_CUR = SEEK_CUR
} io_stream_seek_t;

class io_stream
{
public:
  /* create a new stream from an existing one - used to get
   * decompressed data
   * or open archives.
   * will return NULL if there is no sub-stream available (ie (peek()
   * didn't match any known magic number) && nextfilename () = NULL
   */
  static io_stream *factory (io_stream *);
  /* open a stream by url. The particular stream type returned
   * will depend on the url passed.
   * ie for file:// it will be a disk file.
   * for ftp:// it will perform an upload to a ftp site.
   * the second parameter - mode can specify r|w && t|b. Other flags are not currently
   * supported.
   * Automatic decompression does not occur. Compressed files will return a io_stream
   * from archive::decompress. This behaviour is by design - to allow deliberate access
   * to the compressed data.
   * To create a stream that will be compressed, you should open the url, and then get a new stream
   * from archive::compress. 
   * If a stream is opened for reading, and it's an archive, the next_file_name method
   * will return non-NULL. To access the files within the archive use io_stream::factory
   * to create a new stream that will read from the archive.
   */
  static io_stream *open (const char *, const char *);
  static int remove (const char *);
  static int exists (const char *);
  /* ensure that we have access to the entire path */
  /* Create a directory, and any needed parent directories.
   * returns 1 on failure.
   */
  static int mkpath_p (path_type_t, const char *);
  /* link from, to, type. Returns 1 on failure */
  static int mklink (const char *, const char *, io_stream_link_t);
  /* TODO: we may need two versions of each of these:
     1 for external use - when the path is known
     1 for inline use, for example to set the mtime of a file being written
     into a tarball
   */
  /* set the modification time of a file - returns 1 on failure
   * may distrupt internal state - use after all important io is complete
   */
  virtual int set_mtime (int) = 0;
  /* get the mtime for a file TODO make this a stat(0 style call */
  virtual int get_mtime () = 0;
  /* read data (duh!) */
  virtual ssize_t read (void *buffer, size_t len) = 0;
  /* provide data to (double duh!) */
  virtual ssize_t write (const void *buffer, size_t len) = 0;
  /* read data without removing it from the class's internal buffer */
  virtual ssize_t peek (void *buffer, size_t len) = 0;
  /* ever read the f* functions from libc ? */
  virtual long tell () = 0;
  virtual int seek (long, io_stream_seek_t) = 0;
  /* try guessing this one */
  virtual int error () = 0;
  /* hmm, yet another for the guessing books */
  virtual char *gets (char *, size_t len);
  /* what sort of stream is this?
   * known types are:
   * IO_STREAM_INVALID - not a valid stream.
   * IO_STREAM_STREAM - just another stream.
   * IO_STREAM_COMPRESS - a compressed or compressing stream.
   * IO_STREAM_ARCHIVE - an archive of some sort, with > 0 files.
   * this is a crutch for real runtime type evaluation.
   */
  /* Find out the next stream name -
   * ie for foo.tar.gz, at offset 0, next_file_name = foo.tar
   * for foobar that is an archive, next_file_name is the next
   * extractable filename.
   */
//  virtual const char* next_file_name() = NULL;
  /* if you are still needing these hints... give up now! */
  virtual ~ io_stream ();
};

#endif /* _IO_STREAM_H_ */