/**
 * Copyright (c) 2015 Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __COMMON_INCLUDE_ARA_MAILBOX_H
#define __COMMON_INCLUDE_ARA_MAILBOX_H

/**
 * @brief Synchronously read from local mailbox.
 * @return 0 on success, <0 on internal error, >0 on UniPro error
 */
int read_mailbox(uint32_t *val);
/**
 * @brief Acknowledge that we've read local mailbox, clearing it.
 * @return 0 on success, <0 on internal error, >0 on UniPro error
 */
int ack_mailbox(uint16_t val);
/**
 * @brief Synchronously write to the peer mailbox, polling for it to be cleared
 * once we've written it.
 * @return 0 on success, <0 on internal error, >0 on UniPro error
 */
int write_mailbox(uint32_t val);

/**
 * @brief Abstract out the chip-common parts of advertising readiness.
 */
int advertise_ready(void);

/**
 * @brief return if there is mailbox irq pending
 */
bool is_mailbox_irq_pending(void);

#endif /* __COMMON_INCLUDE_ARA_MAILBOX_H */
