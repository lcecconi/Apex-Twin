APEX ?= $(shell if command -v uv >/dev/null 2>&1; then echo "uv run apex"; elif [ -x .venv/bin/apex ]; then echo ".venv/bin/apex"; else echo "apex"; fi)

.PHONY: help emu flash build monitor util mon test setup dash track

help:
	@$(APEX) --help

emu:
	@$(APEX) emu

dash:
	@$(APEX) dash flash

track:
	@$(APEX) track flash

flash:
	@$(APEX) dash flash

build:
	@$(APEX) dash build

monitor:
	@$(APEX) dash monitor

util:
	@$(APEX) util

test:
	@$(APEX) test

setup:
	@$(APEX) setup

