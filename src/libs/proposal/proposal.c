/**
 * @file
 *
 * @brief Implementation of proposed API enhancements.
 *
 * @copyright BSD License (see doc/COPYING or http://www.libelektra.org)
 *
 */

#include <string.h>

#include <kdbprivate.h>

/**
 * @defgroup proposal Proposals for Elektra
 * @brief Might be added to, changed or removed from future Elektra releases.
 */

/**
 * @defgroup api API Proposals for Elektra
 * @brief for kdb.h.
 * @ingroup proposal
 *
 * @warning Do not use these methods if you do not want to depend on
 * exactly the Elektra version your binary was built for.
 *
 * These methods are a technical preview of what might be added in
 * future Elektra releases. It is a requirement that methods are first
 * added here, before they are added to the public API.
 *
 * Usually, names in proposal stage should be prefixed with elektra to
 * clearly mark that the signature is likely to be changed and not yet
 * ABI compatible.
 *
 * @{
 */


/**
 * @brief Set a formatted string
 *
 * @param key the key to set the string value
 * @param format NULL-terminated text format string
 * @param ... more arguments
 *
 * @return the size of the string as set (with including 0)
 */
ssize_t keySetStringF(Key *key, const char *format, ...)
{
	va_list arg_list;

	keySetMeta (key, "binary", 0);

	va_start(arg_list, format);
	char *p = elektraVFormat(format, arg_list);
	va_end(arg_list);

	if (!p)
	{
		return -1;
	}

	if (key->data.c)
	{
		elektraFree(key->data.c);
	}

	key->data.c = p;
	key->dataSize = elektraStrLen(key->data.c);
	set_bit(key->flags, KEY_FLAG_SYNC);

	return key->dataSize;
}


/**
 * Builds an array of pointers to the keys in the supplied keyset.
 * The keys are not copied, calling keyDel may remove them from
 * the keyset.
 *
 * The size of the buffer can be easily allocated via ksGetSize. Example:
 * @code
 * KeySet *ks = somekeyset;
 * Key **keyArray = calloc (ksGetSize(ks), sizeof (Key *));
 * elektraKsToMemArray (ks, keyArray);
 * ... work with the array ...
 * elektraFree (keyArray);
 * @endcode
 *
 * @param ks the keyset object to work with
 * @param buffer the buffer to put the result into
 * @return the number of elements in the array if successful
 * @return a negative number on null pointers or if an error occurred
 */
int elektraKsToMemArray(KeySet *ks, Key **buffer)
{
	if (!ks) return -1;
	if (!buffer) return -1;

	/* clear the received buffer */
	memset (buffer, 0, ksGetSize (ks) * sizeof(Key *));

	cursor_t cursor = ksGetCursor (ks);
	ksRewind (ks);
	size_t idx = 0;

	Key *key;
	while ((key = ksNext (ks)) != 0)
	{
		buffer[idx] = key;
		++idx;
	}
	ksSetCursor (ks, cursor);

	return idx;
}

/**
 * @brief Takes the first key and cuts off this common part
 * for all other keys, instead name will be prepended
 *
 * @return a new allocated keyset with keys in user namespace.
 *
 * The first key is removed in the resulting keyset.
 */
KeySet* elektraRenameKeys(KeySet *config, const char* name)
{
	Key *root;
	Key *cur;
	ssize_t rootSize = 0;

	ksRewind(config);

	root = ksNext (config);
	rootSize = keyGetNameSize(root);

	keyDel (ksLookup (config, root, KDB_O_POP));

	KeySet *newConfig = ksNew(ksGetSize(config), KS_END);
	if (rootSize == -1) return newConfig;

	while ((cur = ksPop(config)) != 0)
	{
		Key *dupKey = keyDup(cur);
		keySetName(dupKey, name);
		keyAddName(dupKey, keyName(cur)+rootSize-1);
		ksAppendKey(newConfig, dupKey);
		keyDel(cur);
	}

	return newConfig;
}

/**
 * @brief Permanently locks a part of the key
 *
 * This can be:
 * - KEY_FLAG_LOCK_NAME to lock the name
 * - KEY_FLAG_LOCK_VALUE to lock the value
 * - KEY_FLAG_LOCK_META to lock the meta data
 *
 * To unlock the key, duplicate it.
 *
 * It is also possible to lock when the key is created with
 * keyNew().
 *
 * Some data structures need to lock the key (most likely
 * its name), so that the ordering does not get confused.
 *
 * @param key which name should be locked
 *
 * @see keyNew(), keyDup(), ksAppendKey()
 * @retval >0 the bits that were successfully locked
 * @retval 0 if everything was locked before
 * @retval -1 if it could not be locked (nullpointer)
 */
int keyLock(Key *key, /*option_t*/ enum elektraLockOptions what)
{
	int ret = 0;

	if (!key) return -1;

	if (test_bit(what, KEY_LOCK_NAME))
	{
		if (!test_bit(key->flags, KEY_FLAG_RO_NAME))
		{
			set_bit(key->flags, KEY_FLAG_RO_NAME);
			set_bit(ret, KEY_LOCK_NAME);
		}
	}

	if (test_bit(what, KEY_LOCK_VALUE))
	{
		if (!test_bit(key->flags, KEY_FLAG_RO_VALUE))
		{
			set_bit(key->flags, KEY_FLAG_RO_VALUE);
			set_bit(ret, KEY_LOCK_VALUE);
		}
	}

	if (test_bit(what, KEY_LOCK_META))
	{
		if (!test_bit(key->flags, KEY_FLAG_RO_META))
		{
			set_bit(key->flags, KEY_FLAG_RO_META);
			set_bit(ret, KEY_LOCK_META);
		}
	}

	return ret;
}


/**
 * @brief Return meta data as keyset
 *
 * @param key the key object to work with
 *
 * @return a duplication of the keyset representing the meta data
 */
KeySet *elektraKeyGetMetaKeySet(const Key *key)
{
	if (!key) return 0;
	if (!key->meta) return 0;

	return ksDup(key->meta);
}


/**
 * Returns the previous Key in a KeySet.
 *
 * KeySets have an internal cursor that can be reset with ksRewind(). Every
 * time ksPrev() is called the cursor is decremented and the new current Key
 * is returned.
 *
 * You'll get a NULL pointer if the key before begin of the KeySet was reached.
 *
 * Don't delete the key, use ksPop() if you want to delete it.
 *
 * @return the new current Key
 * @see ksRewind(), ksCurrent()
 *
 */
Key *ksPrev(KeySet *ks)
{
	if (ks->size == 0) return 0;
	if (ks->current <= 0)
	{
		ksRewind (ks);
		return 0;
	}
	ks->current--;
	return ks->cursor = ks->array[ks->current];
}

/**
 * @brief Pop key at given cursor position
 *
 * @param ks the keyset to pop key from
 * @param c where to pop
 *
 * The internal cursor will be rewinded using ksRewind(). You can use
 * ksGetCursor() and ksSetCursor() jump back to the previous position.
 * e.g. to pop at current position within ksNext() loop:
 * @code
 * cursor_t c = ksGetCursor(ks);
 * keyDel (ksPopAtCursor(ks, c));
 * ksSetCursor(ks, c);
 * ksPrev(ks); // to have correct key after next ksNext()
 * @endcode
 *
 * @warning do not use, will be superseded by external iterator API
 *
 * @return the popped key
 * @retval 0 if ks is 0
 */
Key *ksPopAtCursor(KeySet *ks, cursor_t pos)
{
	if (!ks) return 0;
	if (pos<0) return 0;
	if (pos>SSIZE_MAX) return 0;

	size_t c = pos;
	if (c>=ks->size) return 0;

	if (c != ks->size-1)
	{
		Key ** found = ks->array+c;
		Key * k = *found;
		/* Move the array over the place where key was found
		 *
		 * e.g. c = 2
		 *   size = 6
		 *
		 * 0  1  2  3  4  5  6
		 * |--|--|c |--|--|--|size
		 * move to (c/pos is overwritten):
		 * |--|--|--|--|--|
		 *
		 * */
		memmove (found,
			found+1,
			(ks->size-c-1) * sizeof(Key *));
		*(ks->array+ks->size-1) = k; // prepare last element to pop
	}
	else
	{
		// if c is on last position it is just a ksPop..
		// so do nothing..
	}

	ksRewind(ks);

	return ksPop(ks);
}

/**
 * @}
 */

