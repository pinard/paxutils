#!/usr/local/bin/perl --					# -*-Perl-*-
eval "exec /usr/local/bin/perl -S $0 $*"
    if 0;

# Process `tar.texi', replacing @value's and @FIXME's appropriately.
# Copyright © 1996, 1998 Progiciels Bourbeau-Pinard inc.
# François Pinard <pinard@iro.umontreal.ca>, 1996.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

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
