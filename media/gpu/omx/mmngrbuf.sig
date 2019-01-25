# Copyright (c) 2019 Renesas Electronics Corporation
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Functions Renesas MMNGR functions used in Chromium code

int mmngr_export_start_in_user_ext(int *pid, size_t size, unsigned int hard_addr, int *pbuf, void *mem_param);
int mmngr_export_end_in_user_ext(int id);
