APEX ?= $(shell if command -v uv >/dev/null 2>&1; then echo "uv run apex"; elif [ -x .venv/bin/apex ]; then echo ".venv/bin/apex"; else echo "apex"; fi)

.PHONY: help emu flash build monitor mon test setup

help:
	@$(APEX) --help

emu:
	@$(APEX) emu

flash:
	@$(APEX) flash

build:
	@$(APEX) build

monitor:
	@$(APEX) monitor

mon:
	@$(APEX) mon

test:
	@$(APEX) test

setup:
	@$(APEX) setup

