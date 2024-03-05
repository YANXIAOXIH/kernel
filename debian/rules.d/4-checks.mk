# Check ABI for package against last release (if not same abinum)
abi-check-%: $(stampdir)/stamp-install-%
	@echo Debug: $@

# Check the module list against the last release (always)
module-check-%: $(stampdir)/stamp-install-%
	@echo Debug: $@

# Check the signature of staging modules
module-signature-check-%: $(stampdir)/stamp-install-%
	@echo Debug: $@

# Check the reptoline jmp/call functions against the last release.
retpoline-check-%: $(stampdir)/stamp-install-%
	@echo Debug: $@

checks-%: module-check-% module-signature-check-% abi-check-% retpoline-check-%
	@echo Debug: $@

# Check the config against the known options list.
config-prepare-check-%: $(stampdir)/stamp-prepare-tree-%
	@echo Debug: $@
