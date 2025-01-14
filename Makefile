ifneq ($(DH_VERBOSE),)
	V ?= 1
endif

ifneq ($(V),)
	MAKEFLAGS += V=$(V)
else
	Q = @
	MAKEFLAGS += --no-print-directory
endif

ifneq ($(KDIR),)
	MAKEFLAGS += KDIR=$(KDIR)
endif

ifneq ($(MO),)
	MAKEFLAGS += MO=$(MO)
endif

# subject, verb, object(s)
svo = $(if $(Q),$(info $1 $3))$(Q)$2 $3

default:

bindeb-pkg:
	$(call svo, DEBUILD,\
		debuild -uc -us -ui -b --lintian-opts --profile debian)

sample-xeth-switchd: FORCE
	$(call svo, GOBUILD,go build ./go/$@)

clean: files = $(wildcard README.html sample-xeth-switchd\
	go/sample-xeth-switchd/sample-xeth-switchd)
clean:
	$(call svo, DKMS, $(MAKE) -C dkms, $@)
	$(if $(files),$(call svo, CLEAN, rm, $(files)))

docs: README.html

modules modules_install:
	$(call svo, DKMS, $(MAKE) -C dkms, $@)

.PHONY: default bindeb-pkg clean docs modules modules_install

%.html: %.md ; pandoc --from gfm --to html -o $@ $<

FORCE: ;
