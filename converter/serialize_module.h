#ifndef DUMP_MODULE_H
#define DUMP_MODULE_H

struct pimp_module;
struct serializer;

void serialize_module(struct serializer *s, const struct pimp_module *mod);

#endif /* DUMP_MODULE_H */
