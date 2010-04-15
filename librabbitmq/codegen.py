# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0
#
# The contents of this file are subject to the Mozilla Public License
# Version 1.1 (the "License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
# the License for the specific language governing rights and
# limitations under the License.
#
# The Original Code is librabbitmq.
#
# The Initial Developers of the Original Code are LShift Ltd, Cohesive
# Financial Technologies LLC, and Rabbit Technologies Ltd.  Portions
# created before 22-Nov-2008 00:00:00 GMT by LShift Ltd, Cohesive
# Financial Technologies LLC, or Rabbit Technologies Ltd are Copyright
# (C) 2007-2008 LShift Ltd, Cohesive Financial Technologies LLC, and
# Rabbit Technologies Ltd.
#
# Portions created by LShift Ltd are Copyright (C) 2007-2009 LShift
# Ltd. Portions created by Cohesive Financial Technologies LLC are
# Copyright (C) 2007-2009 Cohesive Financial Technologies
# LLC. Portions created by Rabbit Technologies Ltd are Copyright (C)
# 2007-2009 Rabbit Technologies Ltd.
#
# Portions created by Tony Garnock-Jones are Copyright (C) 2009-2010
# LShift Ltd and Tony Garnock-Jones.
#
# All Rights Reserved.
#
# Contributor(s): ______________________________________.
#
# Alternatively, the contents of this file may be used under the terms
# of the GNU General Public License Version 2 or later (the "GPL"), in
# which case the provisions of the GPL are applicable instead of those
# above. If you wish to allow use of your version of this file only
# under the terms of the GPL, and not to allow others to use your
# version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the
# notice and other provisions required by the GPL. If you do not
# delete the provisions above, a recipient may use your version of
# this file under the terms of any one of the MPL or the GPL.
#
# ***** END LICENSE BLOCK *****

from __future__ import nested_scopes

from amqp_codegen import *
import string
import re

cTypeMap = {
    'octet': 'uint8_t',
    'shortstr': 'amqp_bytes_t',
    'longstr': 'amqp_bytes_t',
    'short': 'uint16_t',
    'long': 'uint32_t',
    'longlong': 'uint64_t',
    'bit': 'amqp_boolean_t',
    'table': 'amqp_table_t',
    'timestamp': 'uint64_t',
}

def c_ize(s):
    s = s.replace('-', '_')
    s = s.replace(' ', '_')
    return s

AmqpMethod.defName = lambda m: cConstantName(c_ize(m.klass.name) + '_' + c_ize(m.name) + "_method")
AmqpMethod.structName = lambda m: "amqp_" + c_ize(m.klass.name) + '_' + c_ize(m.name) + "_t"

AmqpClass.structName = lambda c: "amqp_" + c_ize(c.name) + "_properties_t"

def cConstantName(s):
    return 'AMQP_' + '_'.join(re.split('[- ]', s.upper()))

def cFlagName(c, f):
    return cConstantName(c.name + '_' + f.name) + '_FLAG'

def genErl(spec):
    def cType(domain):
        return cTypeMap[spec.resolveDomain(domain)]

    def fieldTempList(fields):
        return '[' + ', '.join(['F' + str(f.index) for f in fields]) + ']'

    def fieldMapList(fields):
        return ', '.join([c_ize(f.name) + " = F" + str(f.index) for f in fields])

    def genLookupMethodName(m):
        print '    case %s: return "%s";' % (m.defName(), m.defName())

    def genSingleDecode(prefix, cLvalue, unresolved_domain):
        type = spec.resolveDomain(unresolved_domain)
        if type == 'shortstr':
            print prefix + "%s.len = D_8(encoded, offset);" % (cLvalue,)
            print prefix + "offset++;"
            print prefix + "%s.bytes = D_BYTES(encoded, offset, %s.len);" % (cLvalue, cLvalue)
            print prefix + "offset += %s.len;" % (cLvalue,)
        elif type == 'longstr':
            print prefix + "%s.len = D_32(encoded, offset);" % (cLvalue,)
            print prefix + "offset += 4;"
            print prefix + "%s.bytes = D_BYTES(encoded, offset, %s.len);" % (cLvalue, cLvalue)
            print prefix + "offset += %s.len;" % (cLvalue,)
        elif type == 'octet':
            print prefix + "%s = D_8(encoded, offset);" % (cLvalue,)
            print prefix + "offset++;"
        elif type == 'short':
            print prefix + "%s = D_16(encoded, offset);" % (cLvalue,)
            print prefix + "offset += 2;"
        elif type == 'long':
            print prefix + "%s = D_32(encoded, offset);" % (cLvalue,)
            print prefix + "offset += 4;"
        elif type == 'longlong':
            print prefix + "%s = D_64(encoded, offset);" % (cLvalue,)
            print prefix + "offset += 8;"
        elif type == 'timestamp':
            print prefix + "%s = D_64(encoded, offset);" % (cLvalue,)
            print prefix + "offset += 8;"
        elif type == 'bit':
            raise "Can't decode bit in genSingleDecode"
        elif type == 'table':
            print prefix + "table_result = amqp_decode_table(encoded, pool, &(%s), &offset);" % \
                  (cLvalue,)
            print prefix + "AMQP_CHECK_RESULT(table_result);"
        else:
            raise "Illegal domain in genSingleDecode", type

    def genSingleEncode(prefix, cValue, unresolved_domain):
        type = spec.resolveDomain(unresolved_domain)
        if type == 'shortstr':
            print prefix + "E_8(encoded, offset, %s.len);" % (cValue,)
            print prefix + "offset++;"
            print prefix + "E_BYTES(encoded, offset, %s.len, %s.bytes);" % (cValue, cValue)
            print prefix + "offset += %s.len;" % (cValue,)
        elif type == 'longstr':
            print prefix + "E_32(encoded, offset, %s.len);" % (cValue,)
            print prefix + "offset += 4;"
            print prefix + "E_BYTES(encoded, offset, %s.len, %s.bytes);" % (cValue, cValue)
            print prefix + "offset += %s.len;" % (cValue,)
        elif type == 'octet':
            print prefix + "E_8(encoded, offset, %s);" % (cValue,)
            print prefix + "offset++;"
        elif type == 'short':
            print prefix + "E_16(encoded, offset, %s);" % (cValue,)
            print prefix + "offset += 2;"
        elif type == 'long':
            print prefix + "E_32(encoded, offset, %s);" % (cValue,)
            print prefix + "offset += 4;"
        elif type == 'longlong':
            print prefix + "E_64(encoded, offset, %s);" % (cValue,)
            print prefix + "offset += 8;"
        elif type == 'timestamp':
            print prefix + "E_64(encoded, offset, %s);" % (cValue,)
            print prefix + "offset += 8;"
        elif type == 'bit':
            raise "Can't encode bit in genSingleDecode"
        elif type == 'table':
            print prefix + "table_result = amqp_encode_table(encoded, &(%s), &offset);" % \
                  (cValue,)
            print prefix + "if (table_result < 0) return table_result;"
        else:
            raise "Illegal domain in genSingleEncode", type

    def genDecodeMethodFields(m):
        print "    case %s: {" % (m.defName(),)
        print "      %s *m = (%s *) amqp_pool_alloc(pool, sizeof(%s));" % \
              (m.structName(), m.structName(), m.structName())
        print "      if (m == NULL) { return -ENOMEM; }"
        bitindex = None
        for f in m.arguments:
            if spec.resolveDomain(f.domain) == 'bit':
                if bitindex is None:
                    bitindex = 0
                if bitindex >= 8:
                    bitindex = 0
                if bitindex == 0:
                    print "      bit_buffer = D_8(encoded, offset);"
                    print "      offset++;"
                print "      m->%s = (bit_buffer & (1 << %d)) ? 1 : 0;" % \
                      (c_ize(f.name), bitindex)
                bitindex = bitindex + 1
            else:
                bitindex = None
                genSingleDecode("      ", "m->%s" % (c_ize(f.name),), f.domain)
        print "      *decoded = m;"
        print "      return 0;"
        print "    }"

    def genDecodeProperties(c):
        print "    case %d: {" % (c.index,)
        print "      %s *p = (%s *) amqp_pool_alloc(pool, sizeof(%s));" % \
              (c.structName(), c.structName(), c.structName())
        print "      if (p == NULL) { return -ENOMEM; }"
        print "      p->_flags = flags;"
        for f in c.fields:
            if spec.resolveDomain(f.domain) == 'bit':
                pass
            else:
                print "      if (flags & %s) {" % (cFlagName(c, f),)
                genSingleDecode("        ", "p->%s" % (c_ize(f.name),), f.domain)
                print "      }"
        print "      *decoded = p;"
        print "      return 0;"
        print "    }"

    def genEncodeMethodFields(m):
        print "    case %s: {" % (m.defName(),)
        if m.arguments:
            print "      %s *m = (%s *) decoded;" % (m.structName(), m.structName())
        bitindex = None
        def finishBits():
            if bitindex is not None:
                print "      E_8(encoded, offset, bit_buffer);"
                print "      offset++;"
        for f in m.arguments:
            if spec.resolveDomain(f.domain) == 'bit':
                if bitindex is None:
                    bitindex = 0
                    print "      bit_buffer = 0;"
                if bitindex >= 8:
                    finishBits()
                    print "      bit_buffer = 0;"
                    bitindex = 0
                print "      if (m->%s) { bit_buffer |= (1 << %d); }" % \
                      (c_ize(f.name), bitindex)
                bitindex = bitindex + 1
            else:
                finishBits()
                bitindex = None
                genSingleEncode("      ", "m->%s" % (c_ize(f.name),), f.domain)
        finishBits()
        print "      return offset;"
        print "    }"

    def genEncodeProperties(c):
        print "    case %d: {" % (c.index,)
        if c.fields:
            print "      %s *p = (%s *) decoded;" % (c.structName(), c.structName())
        for f in c.fields:
            if spec.resolveDomain(f.domain) == 'bit':
                pass
            else:
                print "      if (flags & %s) {" % (cFlagName(c, f),)
                genSingleEncode("        ", "p->%s" % (c_ize(f.name),), f.domain)
                print "      }"
        print "      return offset;"
        print "    }"

    methods = spec.allMethods()

    print '/* Autogenerated code. Do not edit. */'
    print
    print '#include <stdlib.h>'
    print '#include <stdint.h>'
    print '#include <string.h>'
    print '#include <stdio.h>'
    print '#include <errno.h>'
    print '#include <arpa/inet.h> /* ntohl, htonl, ntohs, htons */'
    print
    print '#include "amqp.h"'
    print '#include "amqp_framing.h"'
    print '#include "amqp_private.h"'

    print """
char const *amqp_constant_name(int constantNumber) {
  switch (constantNumber) {"""
    for (c,v,cls) in spec.constants:
        print "    case %s: return \"%s\";" % (cConstantName(c), cConstantName(c))
    print """    default: return "(unknown)";
  }
}"""

    print """
amqp_boolean_t amqp_constant_is_hard_error(int constantNumber) {
  switch (constantNumber) {"""
    for (c,v,cls) in spec.constants:
        if cls == 'hard-error':
            print "    case %s: return 1;" % (cConstantName(c),)
    print """    default: return 0;
  }
}"""

    print """
char const *amqp_method_name(amqp_method_number_t methodNumber) {
  switch (methodNumber) {"""
    for m in methods: genLookupMethodName(m)
    print """    default: return NULL;
  }
}"""

    print """
amqp_boolean_t amqp_method_has_content(amqp_method_number_t methodNumber) {
  switch (methodNumber) {"""
    for m in methods:
        if m.hasContent:
            print '    case %s: return 1;' % (m.defName())
    print """    default: return 0;
  }
}"""

    print """
int amqp_decode_method(amqp_method_number_t methodNumber,
                       amqp_pool_t *pool,
                       amqp_bytes_t encoded,
                       void **decoded)
{
  int offset = 0;
  int table_result;
  uint8_t bit_buffer;

  switch (methodNumber) {"""
    for m in methods: genDecodeMethodFields(m)
    print """    default: return -ENOENT;
  }
}"""

    print """
int amqp_decode_properties(uint16_t class_id,
                           amqp_pool_t *pool,
                           amqp_bytes_t encoded,
                           void **decoded)
{
  int offset = 0;
  int table_result;

  amqp_flags_t flags = 0;
  int flagword_index = 0;
  amqp_flags_t partial_flags;

  do {
    partial_flags = D_16(encoded, offset);
    offset += 2;
    flags |= (partial_flags << (flagword_index * 16));
    flagword_index++;
  } while (partial_flags & 1);

  switch (class_id) {"""
    for c in spec.allClasses(): genDecodeProperties(c)
    print """    default: return -ENOENT;
  }
}"""

    print """
int amqp_encode_method(amqp_method_number_t methodNumber,
                       void *decoded,
                       amqp_bytes_t encoded)
{
  int offset = 0;
  int table_result;
  uint8_t bit_buffer;

  switch (methodNumber) {"""
    for m in methods: genEncodeMethodFields(m)
    print """    default: return -ENOENT;
  }
}"""

    print """
int amqp_encode_properties(uint16_t class_id,
                           void *decoded,
                           amqp_bytes_t encoded)
{
  int offset = 0;
  int table_result;

  /* Cheat, and get the flags out generically, relying on the
     similarity of structure between classes */
  amqp_flags_t flags = * (amqp_flags_t *) decoded; /* cheating! */

  {
    /* We take a copy of flags to avoid destroying it, as it is used
       in the autogenerated code below. */
    amqp_flags_t remaining_flags = flags;
    do {
      amqp_flags_t remainder = remaining_flags >> 16;
      uint16_t partial_flags = remaining_flags & 0xFFFE;
      if (remainder != 0) { partial_flags |= 1; }
      E_16(encoded, offset, partial_flags);
      offset += 2;
      remaining_flags = remainder;
    } while (remaining_flags != 0);
  }
  
  switch (class_id) {"""
    for c in spec.allClasses(): genEncodeProperties(c)
    print """    default: return -ENOENT;
  }
}"""

def genHrl(spec):
    def cType(domain):
        return cTypeMap[spec.resolveDomain(domain)]

    def fieldDeclList(fields):
        return ''.join(["  %s %s;\n" % (cType(f.domain), c_ize(f.name)) for f in fields])

    def propDeclList(fields):
        return ''.join(["  %s %s;\n" % (cType(f.domain), c_ize(f.name))
                        for f in fields
                        if spec.resolveDomain(f.domain) != 'bit'])

    methods = spec.allMethods()

    print """/* Autogenerated code. Do not edit. */
#ifndef librabbitmq_amqp_framing_h
#define librabbitmq_amqp_framing_h

#ifdef __cplusplus
extern "C" {
#endif
"""
    print "#define AMQP_PROTOCOL_VERSION_MAJOR %d" % (spec.major)
    print "#define AMQP_PROTOCOL_VERSION_MINOR %d" % (spec.minor)
    print "#define AMQP_PROTOCOL_VERSION_REVISION %d" % (spec.revision)
    print "#define AMQP_PROTOCOL_PORT %d" % (spec.port)

    for (c,v,cls) in spec.constants:
        print "#define %s %s" % (cConstantName(c), v)
    print

    print """/* Function prototypes. */
extern char const *amqp_constant_name(int constantNumber);
extern amqp_boolean_t amqp_constant_is_hard_error(int constantNumber);
extern char const *amqp_method_name(amqp_method_number_t methodNumber);
extern amqp_boolean_t amqp_method_has_content(amqp_method_number_t methodNumber);
extern int amqp_decode_method(amqp_method_number_t methodNumber,
                              amqp_pool_t *pool,
                              amqp_bytes_t encoded,
                              void **decoded);
extern int amqp_decode_properties(uint16_t class_id,
                                  amqp_pool_t *pool,
                                  amqp_bytes_t encoded,
                                  void **decoded);
extern int amqp_encode_method(amqp_method_number_t methodNumber,
                              void *decoded,
                              amqp_bytes_t encoded);
extern int amqp_encode_properties(uint16_t class_id,
                                  void *decoded,
                                  amqp_bytes_t encoded);
"""

    print "/* Method field records. */"
    for m in methods:
        methodid = m.klass.index << 16 | m.index
        print "#define %s ((amqp_method_number_t) 0x%.08X) /* %d, %d; %d */" % \
              (m.defName(),
               methodid,
               m.klass.index,
               m.index,
               methodid)
        print "typedef struct %s_ {\n%s} %s;\n" % \
              (m.structName(), fieldDeclList(m.arguments), m.structName())

    print "/* Class property records. */"
    for c in spec.allClasses():
        print "#define %s (0x%.04X) /* %d */" % \
              (cConstantName(c.name + "_class"), c.index, c.index)
        index = 0
        for f in c.fields:
            if index % 16 == 15:
                index = index + 1
            shortnum = index / 16
            partialindex = 15 - (index % 16)
            bitindex = shortnum * 16 + partialindex
            print '#define %s (1 << %d)' % (cFlagName(c, f), bitindex)
            index = index + 1
        print "typedef struct %s_ {\n  amqp_flags_t _flags;\n%s} %s;\n" % \
              (c.structName(),
               fieldDeclList(c.fields),
               c.structName())

    print """#ifdef __cplusplus
}
#endif

#endif"""

def generateErl(specPath):
    genErl(AmqpSpec(specPath))

def generateHrl(specPath):
    genHrl(AmqpSpec(specPath))
    
if __name__ == "__main__":
    do_main(generateHrl, generateErl)
