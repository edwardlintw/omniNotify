SUBDIRS = mkattlib lib daemon examples

all::
	@(target="export"; $(MakeSubdirs))

export::
	@$(MakeSubdirs)

install::
	@$(MakeSubdirs)
	@(subdirs="include"; $(MakeSubdirs))

install_examples::
	@(subdirs="examples"; $(MakeSubdirs))

ciao::
	@$(MakeSubdirs)

semiclean:
	(subdirs="lib daemon examples"; target="veryclean"; $(MakeSubdirs))
