/* Functions to help coverity do static analysis on rabbitmq-c */

void amqp_abort(const char *fmt, ...)
{
  __coverity_panic__();
}

void die_on_amqp_error(amqp_rpc_reply *r)
{
  __coverity_panic__();
}

void die_on_error(int r)
{
  __coverity_panic__();
}
