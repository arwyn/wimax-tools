/*
 * Linux WiMax
 * User Space API
 *
 *
 * Copyright (C) 2007-2008 Intel Corporation. All rights reserved.
 * Inaky Perez-Gonzalez <inaky.perez-gonzalez@intel.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 *
 */
/**
 * @mainpage
 *
 * This is a simple library to control WiMAX devices through the
 * control API exported by the Linux kernel WiMAX Stack. It provides
 * means to execute functions exported by the stack and to receive its
 * notifications.
 *
 * Because of this, this is a callback oriented library. It is
 * designed to be operated asynchronously and/or in an event loop. For
 * the very simple cases, helpers that implement synchronous
 * functionality are available.
 *
 * This library is provided as a convenience and using it is not
 * required to talk to the WiMAX kernel stack. It is possible to do so
 * by interacting with it over generic netlink.
 *
 * \note this is a very low level library. It does not provide the
 * caller with means to scan, connect, disconnect, etc from a WiMAX
 * network. Said capability is provided by higher level services which
 * might be users of this library.
 *
 * @section conventions Conventions
 *
 * Most function calls return an integer with a negative \a errno
 * error code when there is an error.
 *
 * @section general_usage General usage
 *
 * The first operation to start controlling a WiMAX device is to open
 * a handle for it:
 *
 * @code
 *  struct wimaxll_handle *wmx = wimaxll_open("wmx0");
 * @endcode
 *
 * With an open handle you can execute all the WiMax API
 * operations. When done with the handle, it has to be closed:
 *
 * @code
 *  wimaxll_close(wmx);
 * @endcode
 *
 * If the device is unloaded/disconnected, the handle will be marked
 * as invalid and any operation will fail with -ENODEV.
 *
 * To reset a WiMAX device, use:
 *
 * @code
 *  wimaxll_reset(wmx);
 * @endcode
 *
 * To turn a device \e on or \e off, or to query it's status, use:
 *
 * @code
 *  wimaxll_rfkill(wmx, WIMAX_RF_ON);
 * @endcode
 *
 * WIMAX_RF_ON and WIMAX_RF_OFF turn the radio on and off
 * respectively, using the software switch and return the current
 * status of both switches. WIMAX_RF_QUERY just returns the status of
 * both the \e HW and \e SW switches.
 *
 * See \ref device_management "device management" for more information.
 *
 * \section receiving Receiving notifications from the WiMAX kernel stack
 *
 * The WiMAX kernel stack will broadcast notifications and
 * driver-specific messages to all the user space clients connected to
 * it over the default \e message \e pipe.
 *
 * Different drivers might implement pipes other then default pipe to
 * which applications can listen to.  For more information, see \ref
 * the_messaging_interface "the messaging interface".
 *
 * To listen to said notifications, a library client needs to block
 * waiting for them or set \ref callbacks "callbacks" and integrate into
 * some kind of main loop using \e select() to detect notifications
 * ready from the kernel.
 *
 * Simple example of mainloop integration:
 *
 * @code
 * int fd = wimaxll_msg_fd(wmx);
 * fd_set pipe_fds, read_fds;
 * ...
 * // Main loop
 * FD_ZERO(&pipe_fds);
 * FD_SET(fd, &pipe_fds);
 * while(1) {
 *         read_fds = pipe_fds;
 *         select(FD_SETSIZE, &read_fds, NULL, NULL, NULL);
 *         if (FD_ISSET(fd, &read_fds))
 *                 wimaxll_pipe_read(wmx, wimax_msg_pipe_id(wmx));
 * }
 * @endcode
 *
 * This code will call wimaxll_pipe_read() on the default \e message
 * \e pipe when notifications are available for delivery. Calling said
 * function will execute, for each notification, the callback
 * associated to it.
 *
 * To wait for a \e state \e change notification, for example:
 *
 * @code
 * result = wimaxll_wait_for_state_change(wmx, &old_state, &new_state);
 * @endcode
 *
 * The same thing can be accomplished setting a callback for a state
 * change notification with wimaxll_set_cb_state_change() and then
 * waiting on a main loop. See \ref state_change_group "state changes"
 * for more information.
 *
 * To wait for a (device-specific) message from the driver on the
 * default \e message pipe, an application would use:
 *
 * @code
 * void *msg;
 * ...
 *
 * size = wimaxll_msg_read(wmx, &msg);
 *
 * ... <act on the message>
 *
 * wimaxll_msg_free(msg);            // done with the message
 * @endcode
 *
 * \e msg points to a buffer which contains the message payload as
 * sent by the driver. When done with \a msg it has to be freed with
 * wimaxll_msg_free().
 *
 * As with \e state \e change notifications, a callback can be set
 * that will be executed from a mainloop every time a message is
 * received from a message pipe. See
 * wimaxll_pipe_set_cb_msg_to_user().
 *
 * A message can be sent to the driver over the default \e message
 * pipe with wimaxll_msg_write(). Note you cannot send messages to the
 * driver over other pipes that are not the default \e message pipe.
 *
 * For more details, see \ref the_messaging_interface.
 *
 * @section miscellaneous Miscellaneous
 *
 * @subsection diagnostics Controlling the ouput of diagnostics
 *
 * \e libwimaxll will output messages by default to \a stderr. See
 * \ref diagnostics_group for changing the default destination.
 *
 * @subsection bytesex Endianess conversion
 *
 * The following convenience helpers are provided for endianess
 * conversion:
 *
 * - wimaxll_le32_to_cpu() and wimaxll_cpu_to_le32()
 * - wimaxll_le16_to_cpu() and wimaxll_cpu_to_le16()
 * - wimaxll_swap_16() and wimaxll_swap_32()
 *
 * @section multithreading Multithreading
 *
 * This library is not threaded or locked. The maximum level of
 * paralellism you can do with one handle is:
 *
 * - Functions that can't be executed in parallel when using the same
 *   handle (need to be serialized):
 *   <ul>
 *     <li> wimaxll_msg_write()
 *     <li> wimaxll_rfkill()
 *     <li> wimaxll_pipe_open(), wimaxll_pipe_msg_read(),
 *          wimaxll_pipe_msg_free(), wimaxll_pipe_close()
 *     <li> wimaxll_mc_rx_open(), wimaxll_mc_rx_close(), wimaxll_mc_rx_read()
 *     <li> wimaxll_reset()
 *   </ul>
 *
 * - wimaxll_msg_read(), wimaxll_pipe_msg_read() and wimaxll_mc_rx_read()
 *   can be run in parallel with other functions, except with
 *   themselves.
 *
 * - callbacks are all executed serially; don't call any pipe
 *   management function from inside a callback.
 *
 * - \a wimaxll_swap_*() and \a wimaxll_*cpu*() can all be parallelized.
 */


#ifndef __lib_wimaxll_h__
#define __lib_wimaxll_h__
#include <sys/errno.h>
#include <sys/types.h>
#include <endian.h>
#include <byteswap.h>
#include <stdarg.h>
#include <linux/wimax.h>

/**
 * A WiMax control pipe handle
 *
 * This type is opaque to the user
 */
struct wimaxll_handle;

struct wimaxll_gnl_cb_context;
struct nlattr;


/**
 * \defgroup callbacks Callbacks
 *
 * When notification callbacks are being executed, the processing of
 * notifications from the kernel is effectively blocked by it. Care
 * must be taken not to call blocking functions, especially
 * wimaxll_pipe_read().
 *
 * Callbacks are always passed a pointer to a private context as set
 * by the application. The context is always of type struct
 * wimaxll_gnl_cb_context and is meant to be used wrapped in an
 * application-specific structure where private information can be
 * stored.
 *
 * When an application specific context is created, the
 * wimaxll_gnl_cb_context part of it should be initialized (with
 * WIMAXLL_GNL_CB_CONTEXT_INIT() or
 * wimaxll_gnl_cb_context_init()). Callback functions can safely
 * update it's \e result field with wimaxll_cb_context_set_result().
 */

/**
 * Callback for a \e message \e to \e user generic netlink message
 *
 * A \e driver \e specific message has been received from the kernel;
 * the pointer to the data and the size are passed in \a data and \a
 * size. The callback can access that data, but it's lifetime is valid
 * only while the callback is executing. If it will be accessed later,
 * it has to be copied to a safe location.
 *
 * \note See \ref callbacks callbacks for a set of warnings and
 * guidelines for using callbacks.
 *
 * \param wmx WiMAX device handle
 * \param ctx Context passed by the user with
 *     wimaxll_pipe_set_cb_msg_to_user().
 * \param data Pointer to a buffer with the message data.
 * \param size Size of the buffer
 * \return 0 if it is ok to keep processing messages, -EBUSY if
 *     message processing should stop and control be returned to the
 *     caller.
 *
 * \ingroup the_messaging_interface
 */
typedef int (*wimaxll_msg_to_user_cb_f)(struct wimaxll_handle *wmx,
					struct wimaxll_gnl_cb_context *ctx,
					const char *data, size_t size);

/**
 * Callback for a \e state \e change notification from the WiMAX
 * kernel stack.
 *
 * The WiMAX device has changed state from \a old_state to \a
 * new_state.
 *
 * \note See \ref callbacks callbacks for a set of warnings and
 * guidelines for using callbacks.
 *
 * \param wmx WiMAX device handle
 * \param ctx Context passed by the user with
 *     wimaxll_set_cb_state_change(). This is a pointer to a standard
 *     context structure than can be wrapped in application-specific
 *     ones.
 * \param old_state State the WiMAX device left
 * \param new_state State the WiMAX device entered
 * \return 0 if it is ok to keep processing messages, -EBUSY if
 *     message processing should stop and control be returned to the
 *     caller.
 *
 * \ingroup state_change_group
 */
typedef int (*wimaxll_state_change_cb_f)(
	struct wimaxll_handle *, struct wimaxll_gnl_cb_context *,
	enum wimax_st old_state, enum wimax_st new_state);


/**
 * General structure for storing callback context
 *
 * \ingroup callbacks
 *
 * Callbacks set by the user receive a user-set pointer to a context
 * structure. The user can wrap this struct in a bigger context struct
 * and use wimaxll_container_of() during the callback to obtain its
 * pointer.
 *
 * Usage:
 *
 * \code
 * ...
 * struct wimaxll_handle *wmx;
 * ...
 * struct my_context {
 *         struct wimaxll_gnl_cb_context ctx;
 *         <my data>
 * } my_ctx = {
 *        .ctx = WIMAXLL_GNL_CB_CONTEXT_INIT(wmx),
 *        <my data initialization>
 * };
 * ...
 * wimaxll_set_cb_SOMECALLBACK(wmx, my_callback, &my_ctx.ctx);
 * ...
 * result = wimaxll_pipe_read(wmx);
 * ...
 *
 * // When my_callback() is called
 * my_callback(wmx, ctx, ...)
 * {
 *         struct my_context *my_ctx = wimaxll_container_of(
 *                ctx, struct my_callback, ctx);
 *         ...
 *         // do stuff with my_ctx
 * }
 * \endcode
 *
 * \param wmx WiMAX handle this context refers to (for usage by the
 *     callback).
 * \param result Result of the handling of the message. For usage by
 *     the callback. Should not be set to -EINPROGRESS, as this will
 *     be interpreted by the message handler as no processing was done
 *     on the message.
 *
 * \internal
 *
 * \param msg_done This is used internally to mark when the acks (or
 *     errors) for a message have been received and the message
 *     receiving loop can be considered done.
 */
struct wimaxll_gnl_cb_context {
	struct wimaxll_handle *wmx;
	ssize_t result;
	unsigned msg_done:1;	/* internal */
};


/**
 * Initialize a definition of struct wimaxll_gnl_cb_context
 *
 * \param _wmx pointer to the WiMAX device handle this will be
 *     associated to
 *
 * Use as:
 *
 * \code
 * struct wimaxll_handle *wmx;
 * ...
 * struct wimaxll_gnl_cb_context my_context = WIMAXLL_GNL_CB_CONTEXT_INIT(wmx);
 * \endcode
 *
 * \ingroup callbacks
 */
#define WIMAXLL_GNL_CB_CONTEXT_INIT(_wmx) {	\
	.wmx = (_wmx),				\
	.result = -EINPROGRESS,			\
}


static inline	// ugly workaround for doxygen
/**
 * Initialize a struct wimaxll_gnl_cb_context
 *
 * \param ctx Pointer to the struct wimaxll_gnl_cb_context.
 * \param wmx pointer to the WiMAX device handle this will be
 *     associated to
 *
 * Use as:
 *
 * \code
 * struct wimaxll_handle *wmx;
 * ...
 * struct wimaxll_gnl_cb_context my_context;
 * ...
 * wimaxll_gnl_cb_context(&my_context, wmx);
 * \endcode
 *
 * \ingroup callbacks
 * \fn static void wimaxll_gnl_cb_context_init(struct wimaxll_gnl_cb_context *ctx, struct wimaxll_handle *wmx)
 */
void wimaxll_gnl_cb_context_init(struct wimaxll_gnl_cb_context *ctx,
				 struct wimaxll_handle *wmx)
{
	ctx->wmx = wmx;
	ctx->result = -EINPROGRESS;
}


static inline	// ugly workaround for doxygen
/**
 * Set the result value in a callback context
 *
 * \param ctx Context where to set -- if NULL, no action will be taken
 * \param val value to set for \a result
 *
 * \ingroup callbacks
 * \fn static void wimaxll_cb_context_set_result(struct wimaxll_gnl_cb_context *ctx, int val)
 */
void wimaxll_cb_context_set_result(struct wimaxll_gnl_cb_context *ctx, int val)
{
	if (ctx != NULL && ctx->result == -EINPROGRESS)
		ctx->result = val;
}


/* Basic handle management */
struct wimaxll_handle *wimaxll_open(const char *device_name);
void wimaxll_close(struct wimaxll_handle *);
const char *wimaxll_ifname(const struct wimaxll_handle *);

/* Very low level handling of pipes for reading generic netlink
 * messages from the kernel */
int wimaxll_mc_rx_open(struct wimaxll_handle *, const char *);
int wimaxll_mc_rx_fd(struct wimaxll_handle *, unsigned);
void wimaxll_mc_rx_close(struct wimaxll_handle *, unsigned);
ssize_t wimaxll_mc_rx_read(struct wimaxll_handle *, unsigned);

/* Handling of pipes for generic netlink messages from the kernel */
int wimaxll_pipe_open(struct wimaxll_handle *, const char *);
int wimaxll_pipe_fd(struct wimaxll_handle *, unsigned);
ssize_t wimaxll_pipe_read(struct wimaxll_handle *, unsigned pipe_id);
void wimaxll_pipe_close(struct wimaxll_handle *, unsigned);

/* driver-user messaging interface for all the pipes */
ssize_t wimaxll_pipe_msg_read(struct wimaxll_handle *, unsigned, void **);
void wimaxll_pipe_msg_free(void *);
void wimaxll_pipe_get_cb_msg_to_user(struct wimaxll_handle *, unsigned pipe_id,
				     wimaxll_msg_to_user_cb_f *,
				     struct wimaxll_gnl_cb_context **);
void wimaxll_pipe_set_cb_msg_to_user(struct wimaxll_handle *, unsigned pipe_id,
				     wimaxll_msg_to_user_cb_f,
				     struct wimaxll_gnl_cb_context *);

/* Default (bidirectional) message pipe from the kernel */
int wimaxll_msg_fd(struct wimaxll_handle *);
ssize_t wimaxll_msg_read(struct wimaxll_handle *, void **);
ssize_t wimaxll_msg_write(struct wimaxll_handle *, const void *, size_t);
void wimaxll_msg_free(void *);
unsigned wimaxll_msg_pipe_id(struct wimaxll_handle *);

/* generic API */
int wimaxll_rfkill(struct wimaxll_handle *, enum wimax_rf_state);
int wimaxll_reset(struct wimaxll_handle *);

void wimaxll_get_cb_state_change(
	struct wimaxll_handle *, wimaxll_state_change_cb_f *,
	struct wimaxll_gnl_cb_context **);
void wimaxll_set_cb_state_change(
	struct wimaxll_handle *, wimaxll_state_change_cb_f,
	struct wimaxll_gnl_cb_context *);
ssize_t wimaxll_wait_for_state_change(struct wimaxll_handle *wmx,
				      enum wimax_st *old_state,
				      enum wimax_st *new_state);


/**
 * \defgroup miscellaneous_group Miscellaneous utilities
 */
extern void (*wimaxll_vmsg)(const char *, va_list);
void wimaxll_vmsg_stderr(const char *, va_list);


static inline	// ugly hack for doxygen
/**
 *
 * Swap the nibbles in a 16 bit number.
 *
 * \ingroup miscellaneous_group
 * \fn unsigned short wimaxll_swap_16(unsigned short x)
 */
unsigned short wimaxll_swap_16(unsigned short x)
{
	return bswap_16(x);
}


static inline	// ugly hack for doxygen
/**
 * Swap the nibbles in a 32 bit number.
 *
 * \ingroup miscellaneous_group
 * \fn unsigned long wimaxll_swap_32(unsigned long x)
 */
unsigned long wimaxll_swap_32(unsigned long x)
{
	return bswap_32(x);
}


static inline	// ugly hack for doxygen
/**
 * Convert a little-endian 16 bits to cpu order.
 *
 * \ingroup miscellaneous_group
 * \fn unsigned short wimaxll_cpu_to_le16(unsigned short x)
 */
unsigned short wimaxll_cpu_to_le16(unsigned short x)
{
	unsigned short le16;
#if __BYTE_ORDER == __LITTLE_ENDIAN
	le16 = x;
#elif __BYTE_ORDER == __BIG_ENDIAN
	le16 = wimaxll_swap_16(x);
#else
#error ERROR: unknown byte sex - FIXME
#endif
	return le16;
}


static inline	// ugly hack for doxygen
/**
 * Convert a cpu-order 16 bits to little endian.
 *
 * \ingroup miscellaneous_group
 * \fn unsigned short wimaxll_le16_to_cpu(unsigned short le16)
 */
unsigned short wimaxll_le16_to_cpu(unsigned short le16)
{
	unsigned short cpu;
#if __BYTE_ORDER == __LITTLE_ENDIAN
	cpu = le16;
#elif __BYTE_ORDER == __BIG_ENDIAN
	cpu = wimaxll_swap_16(le16);
#else
#error ERROR: unknown byte sex - FIXME
#endif
	return cpu;
}


static inline	// ugly hack for doxygen
/**
 * Convert a little-endian 32 bits to cpu order.
 *
 * \ingroup miscellaneous_group
 * \fn unsigned long wimaxll_cpu_to_le32(unsigned long x)
 */
unsigned long wimaxll_cpu_to_le32(unsigned long x)
{
	unsigned long le32;
#if __BYTE_ORDER == __LITTLE_ENDIAN
	le32 = x;
#elif __BYTE_ORDER == __BIG_ENDIAN
	le32 = wimaxll_swap_32(x);
#else
#error ERROR: unknown byte sex - FIXME
#endif
	return le32;
}


static inline	// ugly hack for doxygen
/**
 * Convert a cpu-order 32 bits to little endian.
 *
 * \ingroup miscellaneous_group
 * \fn unsigned long wimaxll_le32_to_cpu(unsigned long le32)
 */
unsigned long wimaxll_le32_to_cpu(unsigned long le32)
{
	unsigned long cpu;
#if __BYTE_ORDER == __LITTLE_ENDIAN
	cpu = le32;
#elif __BYTE_ORDER == __BIG_ENDIAN
	cpu = wimaxll_swap_32(le32);
#else
#error ERROR: unknown byte sex - FIXME
#endif
	return cpu;
}

#endif /* #ifndef __lib_wimaxll_h__ */
