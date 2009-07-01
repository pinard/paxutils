#!/usr/local/bin/perl --					# -*-Perl-*-
eval "exec /usr/local/bin/perl -S $0 $*"
    if 0;

# Process `tar.texi', replacing @value's and @FIXME's appropriately.
# Copyright © 1996, 1998 Progiciels Bourbeau-Pinard inc.
# François Pinard <pinard@iro.umontreal.ca>, 1996.

$PROOF = 0;
if ($ARGV[0] eq '-proof')
{
    $PROOF = 1;
    shift;
}

$_ = <>;
while ($_)
{
    %option = () if /^\@node/;

    if (/^\@macro /)
    {
	$_ = <> while ($_ && ! /^\@end macro/);
	$_ = <>;
    }
    elsif (/^\@set ([^ ]+) (.*)/)
    {
	$set{$1} = $2;
	$_ = <>;
    }
    else
    {
	while (/\@value{(([-a-z0-9]*)[^\}]*)}/)
	{
	    if (defined $set{"$1"})
	    {
		$value = $set{"$1"};

		if ($2)
		{
		    if ($option{$2})
		    {
			$_ = "$`$option{$2}$'";
		    }
		    else
		    {
			$_ = "$`$value$'";
			$option{$2} = "\@w{\@kbd{--$2}}";
		    }
		}
		else
		{
		    $_ = "$`$value$'";
		}
	    }
	    elsif ($PROOF)
	    {
		$_ = "$`\@strong{<UNDEFINED>}$1\@strong{</>}$'";
	    }
	    else
	    {
		$_ = "$`$1$'";
	    }
	}

	while (/\@FIXME\{/)
	{
	    print "$`\@strong{<FIXME>}" if $PROOF;
	    $_ = $';
	    $level = 1;
	    while ($level > 0)
	    {
		if (/([{}])/)
		{
		    if ($1 eq '{')
		    {
			$level++;
			print "$`\{" if $PROOF;
			$_ = $';
		    }
		    elsif ($level > 1)
		    {
			$level--;
			print "$`\}" if $PROOF;
			$_ = $';
		    }
		    else
		    {
			$level = 0;
			print "$`\@strong{</>}" if $PROOF;
			$_ = $';
		    }
		}
		else
		{
		    print if $PROOF;
		    $_ = <>;
		}
	    }
	}

	print;
	$_ = <>;
    }
}

exit 0;
