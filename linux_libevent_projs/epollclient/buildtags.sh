#!/usr/bin/bash
ctags -R -I __THROW -I __attribute_pure -I __nonull -I __attribute__ --file-scope=yes --langmap=c:+.h --languages=c,c++ --links=yes --c-kinds=+p --c++-kinds=+p --fields=+iaS --extra=+q .
cscope -Rbq -f epollclient.out
