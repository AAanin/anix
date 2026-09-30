CC ?= cc
CFLAGS = -std=c99 -ffreestanding -Wall -Wextra -Werror -pedantic -O2
PYTHON ?= python
.PHONY: build test clean
build:
	$(PYTHON) tools/check.py build --cc "$(CC)"
test:
	$(PYTHON) tools/check.py test --cc "$(CC)"
clean:
	$(PYTHON) tools/check.py clean
