/**
 * @file
 * @ingroup kdbio
 *
 * @brief Elektra-I/O functions and declarations for the I/O binding test suite
 *
 * @copyright BSD License (see LICENSE.md or https://www.libelektra.org)
 */
#ifndef KDB_IO_PLUGIN_H_
#define KDB_IO_PLUGIN_H_

#include "kdbio.h"
#include "kdbplugin.h"

#ifdef __cplusplus
namespace ckdb
{
extern "C" {
#endif

/**
 * Set I/O binding for asynchronous I/O operations for plugin.
 *
 * Implemented by plugins to set the I/O binding.
 *
 * @param  plugin    plugin handle
 * @param  ioBinding I/O binding
 */
typedef void (*ElektraIoPluginSetBinding) (Plugin * plugin, ElektraIoInterface * ioBinding);

#ifdef __cplusplus
}
}
#endif

#endif
