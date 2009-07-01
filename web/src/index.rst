====================================
Welcome page for |package| |version|
====================================

.. role:: charset(strong)
.. role:: code(strong)
.. role:: dfn(emphasis)
.. role:: file(literal)
.. role:: kbd(literal)
.. role:: key(strong)
.. role:: var(emphasis)

.. |package| replace:: paxutils
.. |version| replace:: 2.4i

.. contents::
.. sectnum::

**This is the copy of an old, obsolete site which is presented here
merely as a public archive.  Most of the contained information is not
current.**

The original contents of this entry page read like this:

-------

The Free pax utilities package offers tape and disk archivers.

:code:`tar` 1.12 can be considered stable.

You might also want to look at some fine external pointers__, to
discover either complementary or competing packages.  The descriptions
are usually limited to free packages, however.

__ contrib.html

-------

This archive site became possible after Sergey Poznyakoff got from Paul
Eggert, on 2004-03, a copy of all administrative files I (François
Pinard) transmitted to Paul on 1999-07, when Paul officially took over
:code:`tar` and :code:`paxutils` responsibilities.  The main purpose of
this site, as of today, is to ease the communication with the current
:code:`tar` and :code:`paxutils` maintainer, Sergey Poznyakoff, in
view of salvaging some worth old contributions from users that were
ignored in the past few years by the previous :code:`tar` maintainer.
The remainder of this Web page is essentially copied from a letter to
Richard Stallman and Sergey, sent on 2003-12-18, while trying to expose
the current situation of :code:`paxutils` as I perceive or remember it.

-------

It has been a good while since I worked on :code:`paxutils` matters.
All my *ideas* on the matter, including the overall plans on which
we agreed, are documented (so far that I remember) within all
administrative files and folders I transmitted to Paul Eggert, together
with all :code:`paxutils` source in their latest work state, which did
hold extensive changes since the latest :code:`paxutils` pretest.

As a way to cut on all the confusion created by the parallel releases
of :code:`tar` by Paul while :code:`paxutils` was being pretested, and
to fully resist the temptation of ever interfering with Paul's later
decisions and works about :code:`paxutils` (I do not do to others what
I would not like others to do to me), I deleted everything related to
:code:`paxutils` on my side, once Paul told me he fully secured all
the stock I sent him.  So, I do not have anything concrete left of
:code:`paxutils`, besides my fading memory of the project.

This is only later, when I progressively realised that Paul was not
to fulfil his repeated promises about unifying *his* :code:`tar` with
:code:`paxutils` that I became sad about all the work that was being
ignored and lost.  I felt especially bad for all those contributors
to which I did announce that their contributions was accepted and
integrated for the next release of :code:`paxutils`.  I never ever
intended to lie to them.

I'm not sure, but maybe the most fruitful initial move for Sergey would
be to obtain from Paul at least a copy of all the :code:`paxutils`
sources, folders and administrative files I sent him.  I'm pretty sure
all the information is in there.

About :code:`tar`, :code:`cpio` and :code:`paxutils` as separate
projects, a bit of history might help at sorting things.  John Oleynick
maintained :code:`cpio` for a good while, and the transmission of
:code:`cpio` from John to Tom Tromey has been a long, but successful
adventure.  At the time, Tom and I were working at the design of
Automake (after an idea from David Mackenzie) and were intimately
collaborating, and we considered with great pleasure the idea of merging
:code:`tar` and :code:`cpio` together.  We added a newly rewritten
:code:`pax` on the way, and chose :code:`paxutils` as a neutral name
between :code:`tar` and :code:`cpio`.  (We also considered other
versions of :code:`cpio`, :code:`tar` and :code:`pax` -- all our notes
should be in the administrative files).

The :code:`cpio` in :code:`paxutils` was really the most up-to-date,
and was reworked for better integration with the rest, putting code
in common, uniformising style, etc.  Restarting from the latest
:code:`cpio` distribution would clearly be a backward step, I do not
think there is anything to gain there.

:code:`tar` itself has a more complex story.  Paul started from an
old pretest of :code:`paxutils`, modified it heavily on his side on
many aspects, implementing huge file support an a very Solarishly way
(on which I was not much agreeing, should I say).  He then submitted
me a single, big all-mixed, intervowen, unsorted patch.  He probably
expected me to apply it whole, while I wanted to consider each topic of
the patch separately, and told him it would take a good amount of time
to study and process it all.  Paul asked me if I would object that he
publishes :code:`tar` with his patch, separately, for people in urgent
need of using LFS.  That was OK with me.  Then, Paul started not only
to *maintain* his patch, but also reimplement in his :code:`tar` things
that were already solved in the :code:`tar` distributed within later
:code:`paxutils`, and differently.  He went as far as **removing**
from his :code:`tar` good amount of code I wrote in view of large
file support on legacy systems, but which was not aligned along Sun
Solaris methods, which Paul venerates.  In my views, Paul challenged and
dismissed my maintenance works, so much that I thought preferable to
resign than impose our fight to the community.

What it means in practice is that the current :code:`tar` has been
forked from an old pretest of :code:`paxutils`, and so, a merging of
both would be useful, *given* you can get a hold on :code:`paxutils`.
I'm sure that Paul made good works that we cannot ignore (Paul is
technically competent, there is no doubt about this).  I also think
that :code:`paxutils` has good code and changes to recycle.  Merging,
if ever, is going to require courageous work, fairly tedious and
unrewarding.  In my opinion, many months of work need to be identified
and redone.  Paul despisingly told that for him, two or three days would
suffice, but apparently never found those few days.  In the meantime,
Paul acted a bit as if whatever users do not resubmit directly to him
was not worth anyway.

Repairing POSIX support in :code:`tar` is the main long term goal
that :code:`tar` may have.  I guess that the step-wise plan for
:code:`paxutils`, which was accepted by the FSF, has been fully
abandoned or forgotten with the change of maintainer.  But any plan
that has some chance to work is good, in my opinion. :code:`star` is
also especially interesting in that it uses efficient techniques for
streaming tapes, which techniques could be recycled for most or all
:code:`paxutils` tools.

All this being said, Sergey, I can only wish you good courage, and
collaboration from all involved people.  I cannot provide much myself,
besides some bits of historical background, which may not be that useful
in practice, once you will have found all the magnetic stuff you need.
On the other hand, do not hesitate writing to me if you think that
some discussion might help, yet while I did my best while being there,
you understand I have no responsibility anymore around :code:`tar` or
:code:`paxutils`.
