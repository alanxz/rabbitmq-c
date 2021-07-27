// Copyright 2007 - 2021, Alan Antonuk and the rabbitmq-c contributors.
// SPDX-License-Identifier: mit

#ifndef AMQP_OPENSSL_BIO
#define AMQP_OPENSSL_BIO

#include <openssl/bio.h>

int amqp_openssl_bio_init(void);

void amqp_openssl_bio_destroy(void);

typedef const BIO_METHOD *BIO_METHOD_PTR;

BIO_METHOD_PTR amqp_openssl_bio(void);

#endif /* ifndef AMQP_OPENSSL_BIO */
