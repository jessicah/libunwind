/* libunwind - a platform-independent unwind library
   Copyright (C) 2003-2005 Hewlett-Packard Co
	Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

This file is part of libunwind.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  */

#if defined(__HAIKU__)

#include <kernel/image.h>
#include <kernel/OS.h>
#include <runtime_loader.h>

#include <dlfcn.h>
#include <link.h>

#include "libunwind_i.h"

typedef int (*unw_iterate_phdr_impl) (int (*callback) (
                                        struct dl_phdr_info *info,
                                        size_t size, void *data),
                                      void *data);

/* not supported on Haiku, the elf program headers are not mapped */
HIDDEN int
dl_iterate_phdr (int (*callback) (struct dl_phdr_info *info, size_t size, void *data),
                 void *data)
{
  ssize_t cookie;
  int ret = 0;
  
  team_info teamInfo;
  area_info areaInfo;
  
  runtime_loader_debug_area *debugArea;
  image_t *image;
  
  struct dl_phdr_info headerInfo;
  Elf_Phdr *header;
  
  if (get_team_info(B_CURRENT_TEAM, &teamInfo) != B_OK)
  	return -42;
  
  while (get_next_area_info(teamInfo.team, &cookie, &areaInfo) == B_OK) {
  	if (strcmp(areaInfo.name, RUNTIME_LOADER_DEBUG_AREA_NAME) != 0)
  	  return -4;
  	break;
  }
  
  debugArea = (runtime_loader_debug_area*)areaInfo.address;
  
  image = debugArea->loaded_images->head;
  
  while (image != debugArea->loaded_images->tail && image != NULL) {
  	header = (Elf_Phdr*)calloc(image->num_regions, sizeof(Elf_Phdr));
  	
  	headerInfo.dlpi_addr = 0;
  	headerInfo.dlpi_name = image->name;
  	headerInfo.dlpi_phdr = header;
  	headerInfo.dlpi_phnum = image->num_regions;
  	
  	for (uint32 i = 0; i < image->num_regions; ++i, ++header) {
  	  header->p_type = PT_LOAD; // the only type stored in image->regions[]
  	  header->p_offset = image->regions[i].fdstart;
  	  header->p_vaddr = image->regions[i].start;
  	  header->p_paddr = 0; // don't have this available
  	  header->p_filesz = image->regions[i].fdsize;
  	  header->p_memsz = image->regions[i].size;
  	  header->p_flags = image->regions[i].flags & 0x10 ? PF_READ | PF_WRITE : PF_READ;
  	  header->p_align = 0; // not available either
  	}
  	
  	ret = callback(&headerInfo, sizeof(headerInfo), data);
  	
  	free(header);
  	
  	image = image->next;
  }
  
  return ret;
}

#endif
