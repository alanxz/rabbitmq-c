#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <inttypes.h>

#include <rabbitmq-c/amqp.h>

extern int LLVMFuzzerTestOneInput(const char *data, size_t size) {

	struct amqp_connection_info ci;
	int res;
	res = amqp_parse_url((char *)data, &ci);
	return res;
}
