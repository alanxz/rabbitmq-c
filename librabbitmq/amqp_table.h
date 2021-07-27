// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

#ifndef AMQP_TABLE_H
#define AMQP_TABLE_H

#include "amqp_private.h"
#include "rabbitmq-c/amqp.h"

/**
 * Initializes a table entry with utf-8 string type value.
 *
 * \param [in] key the table entry key. The string must remain valid for the
 * life of the resulting amqp_table_entry_t.
 * \param [in] value the string value. The string must remain valid for the life
 * of the resulting amqp_table_entry_t.
 * \returns An initialized table entry.
 */
amqp_table_entry_t amqp_table_construct_utf8_entry(const char *key,
                                                   const char *value);

/**
 * Initializes a table entry with table type value.
 *
 * \param [in] key the table entry key. The string must remain value for the
 * life of the resulting amqp_table_entry_t.
 * \param [in] value the amqp_table_t value. The table must remain valid for the
 * life of the resulting amqp_table_entry_t.
 * \returns An initialized table entry.
 */
amqp_table_entry_t amqp_table_construct_table_entry(const char *key,
                                                    const amqp_table_t *value);

/**
 * Initializes a table entry with boolean type value.
 *
 * \param [in] key the table entry key. The string must remain value for the
 * life of the resulting amqp_table_entry_t.
 * \param [in] value the boolean value. 0 means false, any other value is true.
 * \returns An initialized table entry.
 */
amqp_table_entry_t amqp_table_construct_bool_entry(const char *key,
                                                   const int value);

/**
 * Searches a table for an entry with a matching key.
 *
 * \param [in] table the table to search.
 * \param [in] key the string to search with.
 * \returns a pointer to the table entry in the table if a matching key can be
 * found, NULL otherwise.
 */
amqp_table_entry_t *amqp_table_get_entry_by_key(const amqp_table_t *table,
                                                const amqp_bytes_t key);

#endif /* AMQP_TABLE_H */
