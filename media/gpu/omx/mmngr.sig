# Copyright (c) 2019 Renesas Electronics Corporation
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Functions Renesas MMNGR functions used in Chromium code

int mmngr_alloc_in_user_ext(MMNGR_ID *pid, size_t size, unsigned int *phard_addr, void **puser_virt_addr, unsigned int flag, void *mem_param);
int mmngr_free_in_user_ext(MMNGR_ID id);
