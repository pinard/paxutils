# site-specific parameters for file system backup.

# User name of administrator of backups.
ADMINISTRATOR=friedman

# Hour at which backups are normally done.
# This should be a number from 0 to 23.
BACKUP_HOUR=1

# Device to use for dumping.  It should be on the host
# on which the dump scripts are run.
TAPE_FILE=/dev/nrsmt0

# Command to obtain status of tape drive, including error count.
# On some tape drives there may not be such a command;
# then simply use `TAPE_STATUS=false'.
TAPE_STATUS="mts -t $TAPE_FILE"

# Blocking factor to use for writing the dump.
BLOCKING=124

# List of file systems to be dumped.
# Actually, any directory may be used,
# but if it has subdirectories on other file systems,
# they are not included.

# The host name specifies which host to run tar on.
# It should normally be the host that actually has the file system.
# If GNU tar is not installed on that machine,
# then you can specify some other host which can access
# the file system through NFS.
# Although these are arranged one per line, that is not mandatory.
# It does not work to use # for comments within the string.
BACKUP_DIRS="
	albert:/fs/fsf
	apple-gunkies:/gd
	albert:/fs/gd2
	godwin:/fs/gp
	geech:/usr/jla
	churchy:/usr/roland
	albert:/
	albert:/usr
	apple-gunkies:/
	apple-gunkies:/usr
	ernst:/usr1
	gnu:/
	gnu:/usr
	godwin:/
	apple-gunkies:/com/mailer/gnu
	apple-gunkies:/com/archive/gnu"

# List of individual files to be dumped.
# These should be accesible from the machine on which the dump is run.
BACKUP_FILES="/com/mailer/aliases /com/mailer/league*[a-z]"
