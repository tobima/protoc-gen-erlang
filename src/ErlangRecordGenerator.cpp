#include <iostream>
#include <sstream>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/printer.h>

#include "ErlangUtils.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace erlang {

using google::protobuf::FileDescriptor;
using google::protobuf::FieldDescriptor;
using google::protobuf::Descriptor;
using google::protobuf::io::ZeroCopyOutputStream;
using google::protobuf::io::Printer;
namespace {
string default_value_for_field(const FieldDescriptor* field)
{
  stringstream ss;
  if (field->is_repeated()) ss << "[";
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_DOUBLE:
      ss << field->default_value_double();
      break;
    case FieldDescriptor::CPPTYPE_FLOAT:
      ss << field->default_value_float();
      break;
    case FieldDescriptor::CPPTYPE_INT32:
      ss << field->default_value_int32();
      break;
    case FieldDescriptor::CPPTYPE_UINT32:
      ss << field->default_value_uint32();
      break;
    case FieldDescriptor::CPPTYPE_INT64:
      ss << field->default_value_int64();
      break;
    case FieldDescriptor::CPPTYPE_UINT64:
      ss << field->default_value_uint64();
      break;
    case FieldDescriptor::CPPTYPE_BOOL:
      ss << (field->default_value_bool() ? "true" : "false");
      break;
    case FieldDescriptor::CPPTYPE_STRING:
      ss << "<<\"" << field->default_value_string() << "\">>";
      break;
    case FieldDescriptor::CPPTYPE_ENUM:
      ss << enum_value(field->default_value_enum());
      break;
    case FieldDescriptor::CPPTYPE_MESSAGE:
      ss << "!DEFAULT_VALUE_FOR_MESSAGE_FIELD_IS_NOT_SUPPORTED!";
      break;
  }
  if (field->is_repeated()) ss << "]";
  return ss.str();
}

void enum_to_typespec(Printer& out, const EnumDescriptor* enum_type)
{
  // edoc specification
  out.Print("%% @type $enum$() = ","enum", enum_name(enum_type));
  for(int enumI=0;enumI < enum_type->value_count();enumI++)
  {
    out.Print("$s$","s",to_atom(enum_type->value(enumI)->name()));
    if(enumI < enum_type->value_count()-1)
      out.Print("$s$","s"," | ");
  }
  out.Print(".\n");
  // -type specification
  out.Print("-type $enum$() :: ","enum", enum_name(enum_type));
  for(int enumI=0;enumI < enum_type->value_count();enumI++)
  {
    out.Print("$s$","s",to_atom(enum_type->value(enumI)->name()));
    if(enumI < enum_type->value_count()-1)
      out.Print("$s$","s"," | ");
  }
  out.Print(".\n\n");
}


void message_to_record(Printer& out,const Descriptor* msg)
{
  std::cerr << "Processing " << msg->full_name() << "..." << std::endl;

  for (int i = 0; i < msg->enum_type_count(); i++) {
    enum_to_typespec(out,msg->enum_type(i));
  }

  for(int i=0; i< msg->nested_type_count(); ++i)
    message_to_record(out,msg->nested_type(i));

  // print the edoc information
  std::string scoped=normalized_scope(msg);

  out.Print("%% @type $name$() = #$name${\n",
      "name",to_atom(scoped + "_record"));
  for(int j=0; j < msg->field_count();++j)
  {
    const FieldDescriptor* fd=msg->field(j);
    out.Print("%%   $field$() = $type$","field",to_atom(fd->name()),"type",to_erlang_typespec(fd));
    if(j < msg->field_count()-1)
      out.Print(",\n");
    else
      out.Print("\n%% }.\n");
  }

  // print the record definition
  out.Print("-record($name$,{","name", to_atom(scoped));

  string fn;

  int count = msg->field_count();
  for (int i = 0; i < count; i++) {
    const FieldDescriptor* field = msg->field(i);
    fn = field_name(field);

    out.Print("\n");

    if (field->has_default_value()) {
      out.Print("  $field_name$ = $default$ :: $type$", "field_name", fn, "default", default_value_for_field(field),"type",to_erlang_typespec(field));
    } else if(field-> is_repeated()){
      out.Print("  $field_name$ = [] :: $type$", "field_name", fn,"type",to_erlang_typespec(field));
    } else {
      out.Print("  $field_name$ :: $type$" , "field_name",fn,"type", to_erlang_typespec(field));
    }

    if (i < count-1)
      out.PrintRaw(",");
  }

  out.Print("}).\n\n");
  std::cerr << " done!" <<  std::endl;
}
} // anon namespace

void generate_header(Printer& out, const FileDescriptor* file)
{
  for (int i = 0; i < file->enum_type_count(); i++) {
    enum_to_typespec(out,file->enum_type(i));
  }

  for(int i=0; i < file->message_type_count();++i) {
    message_to_record(out,file->message_type(i));
  }
}
}}}} // namespace google::protobuf::compiler::erlang