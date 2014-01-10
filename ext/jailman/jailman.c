#include <ruby.h>

static VALUE create_namespace_for(VALUE self, VALUE command, VALUE args){
  printf("Hi from Kernel pskhjsdkhsd\n");
  return Qtrue;
}

void Init_jailman(){
  VALUE mJailman       = rb_define_module("Jailman");
  VALUE mLinux         = rb_define_class_under(mJailman, "Linux", rb_cObject);
  rb_define_singleton_method(mLinux, "create_namespace_for", create_namespace_for, 2);
}
