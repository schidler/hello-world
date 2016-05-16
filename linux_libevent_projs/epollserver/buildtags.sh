#!/usr/bin/bash
ctags -R --c++-kinds=+p --fields=+iaS --extra=+q .
cscope -Rbq -f epollserver.out
