��    �      �  �   <      �  +   �  K   %  ?   q  -  �  �  �  �   h  �  �  7  �       !   .  7   P  +   �  �   �     k     z  �   �        	   9  "   C     f  2   �     �     �  $   �          0     H     `     |     �     �  #   �  "   �          "     :     S     i     �     �     �     �  2   �       (   ,  "   U     x     �  "   �  5   �     �               <     Z     m     �     �     �     �  <   �          ,     C     \     p     �     �  "   �  $   �     �     	            1      O      ^      r   !   �   +   �   !   �   #   �   "   !     A!  #   T!     x!     �!     �!  "   �!     �!  0   "  A   3"     u"  #   �"  "   �"  (   �"     #  ,   %#     R#     f#      s#     �#     �#     �#     �#     �#     $  ,   $     H$     \$     t$  2   �$     �$  ,   �$     �$  Y  	%     c&     |&     �&     �&     �&     �&     �&     �&  +   '     H'     ^'     t'     �'     �'     �'  1   �'  )   �'  /   '(  2   W(  4   �(  (   �(  1   �(  %   )  0   @)  +   q)  1   �)     �)     �)     �)     	*  %   *  (   ;*  <   d*     �*     �*     �*     �*     �*  %   �*  �    +     �+     �+     �+  &   ,     .,     :,     X,     m,     �,     �,     �,  !   �,     �,     -  +   -  3   E-     y-     �-     �-     �-     �-     �-     �-  2   �.  K   1/  ?   }/  3  �/  �  �0  �   s3  �  �3  w  �6     R8  !   j8  =   �8  3   �8  �   �8     �9     �9  �   �9     ~:     �:  %   �:  "   �:  1   �:      ;     @;  2   ^;     �;     �;  )   �;     �;     <     "<     ><  (   [<  0   �<  #   �<  !   �<  "   �<     =      >=     _=     x=     �=     �=  7   �=     �=  /   >  $   F>     k>     �>     �>  /   �>     �>     ?  %   ?     ??     _?     ?     �?     �?     �?      �?  ?   @     L@     a@     @     �@     �@      �@     �@  ,   A  )   1A     [A     xA     �A  '   �A  '   �A  !   �A  0   B  ,   MB  9   zB  .   �B  /   �B  0   C     DC  )   YC  )   �C  *   �C     �C      �C     D  ,   2D  B   _D  +   �D  /   �D  2   �D  1   1E  '   cE  0   �E     �E     �E     �E     �E  #   F     9F     NF     [F     vF  "   �F     �F     �F     �F  @   G     EG  0   YG     �G  W  �G     �H  !   I     *I     7I     SI     nI  ,   �I     �I  '   �I     �I     J     ,J  !   HJ     jJ     �J  7   �J  0   �J  5    K  3   6K  9   jK  .   �K  5   �K  "   	L  >   ,L  &   kL  0   �L     �L     �L     �L  
   M  +   M  .   ?M  7   nM     �M     �M  "   �M     �M      N     0N  �   PN     O     O  !   6O  4   XO     �O  &   �O     �O     �O  
   �O     �O  !   P  #   6P     ZP     vP  5   �P  3   �P     �P     Q     7Q     =Q     DQ     RQ     �          4       �   @          x               �       u       W   Z   q   N   �   �   Y   =           s   �   j   �   �   2   ]       �   H   w   �   t   P   T   �   �   L          X   �   9          �   �       #   �   i   8           �   �   /       d      �          f       �   7   `   R       !   (   �   �   �       D   V                 ,       �          �   .   \   *       6   I              '   |      v           �   +   n   C   0   A               �       �   -   �   �       S       �          _   G      m   >   �   b       B   �      
   ;          l   o         �   F      :                   3   �   k   <      ?       �   z   �   �   "      )             U   �       �   ~   1      M   y          J       Q   �   p              �   E       �   }   ^   h          	   a   {   g   5   �   e         [       �       �   �           K   r   �   $   �   �   &           %   O   c                   �       Prepare volume #%d for %s and hit return:  
Copyright (C) 1988, 92, 93, 94, 95, 96, 97 Free Software Foundation, Inc.
 
Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.
 
Device blocking:
  -b, --blocking-factor=BLOCKS   BLOCKS x 512 bytes per record
      --record-size=SIZE         SIZE bytes per record, multiple of 512
  -i, --ignore-zeros             ignore zeroed blocks in archive (means EOF)
  -B, --read-full-records        reblock as we read (for 4.2BSD pipes)
 
Device selection and switching:
  -f, --file=ARCHIVE             use archive file or device ARCHIVE
      --force-local              archive file is local even if has a colon
      --rsh-command=COMMAND      use remote COMMAND instead of rsh
  -[0-7][lmh]                    specify drive and density
  -M, --multi-volume             create/list/extract multi-volume archive
  -L, --tape-length=NUM          change tape after writing NUM x 1024 bytes
  -F, --info-script=FILE         run script at end of each tape (implies -M)
      --new-volume-script=FILE   same as -F FILE
      --volno-file=FILE          use/update the volume number in FILE
 
If a long option shows an argument as mandatory, then it is mandatory
for the equivalent short option also.  Similarly for optional arguments.
 
Local file selection:
  -C, --directory=DIR          change to directory DIR
  -T, --files-from=NAME        get names to extract or create from file NAME
      --null                   -T reads null-terminated names, disable -C
      --exclude=PATTERN        exclude files, given as a globbing PATTERN
  -X, --exclude-from=FILE      exclude globbing patterns listed in FILE
  -P, --absolute-names         don't strip leading `/'s from file names
  -h, --dereference            dump instead the files symlinks point to
      --no-recursion           avoid descending automatically in directories
  -l, --one-file-system        stay in local file system when creating archive
  -K, --starting-file=NAME     begin at file NAME in the archive
 
The backup suffix is `~', unless set with --suffix or SIMPLE_BACKUP_SUFFIX.
The version control may be set with --backup or VERSION_CONTROL, values are:

  t, numbered     make numbered backups
  nil, existing   numbered if numbered backups exist, simple otherwise
  never, simple   always make simple backups
 
Usage: %s [OPTION]...
 
Usage: %s [OPTION]... [FILE]...
 
Written by Fran�ois Pinard <pinard@iro.umontreal.ca>.
 
Written by John Gilmore and Jay Fenlason.
   -N, --newer=DATE             only store files newer than DATE
      --newer-mtime            compare date and time when data changed only
      --after-date=DATE        same as -N
  (core dumped)  link to %s
  n [name]   Give a new file name for the next (and subsequent) volume(s)
 q          Abort tar
 !          Spawn a subshell
 ?          Print this list
  unknown file type `%c'
 %d at %d
 %s is not continued on this volume %s is the archive; not dumped %s: Could not change access and modification times %s: Could not create directory %s: Could not create file %s: Could not create symlink to `%s' %s: Could not link to `%s' %s: Could not make fifo %s: Could not make node %s: Could not write to file %s: Deleting %s
 %s: Error while closing %s: Not found in archive %s: Unknown file type; file ignored %s: Was unable to backup this file %s: is unchanged; not dumped ((child)) Pipe to stdin ((child)) Pipe to stdout (child) Pipe to stdin (child) Pipe to stdout (grandchild) Pipe to stdin (grandchild) Pipe to stdout --Mangled file names--
 --Volume Header--
 Added write and execute permission to directory %s Ambiguous pattern `%s' Amount actually written is (I hope) %d.
 Archive not labelled to match `%s' Archive to stdin Archive to stdout At beginning of tape, quitting now Attempting extraction of symbolic links as hard links Cannot add directory %s Cannot add file %s Cannot allocate buffer space Cannot change to directory %s Cannot chdir to %s Cannot close descriptor %d Cannot close file #%d Cannot exec %s Cannot exec a shell %s Cannot execute remote shell Cannot extract `%s' -- file is continued from another volume Cannot open %s Cannot open archive %s Cannot open directory %s Cannot open file %s Cannot open pipe Cannot properly duplicate %s Cannot read %s Cannot read confirmation from user Cannot read from compression program Cannot read link %s Cannot remove %s Cannot rename %s to %s Cannot seek to %ld in file %s Cannot stat %s Cannot stat file %s Cannot symlink %s to %s Cannot update compressed archives Cannot use multi-volume compressed archives Cannot verify compressed archives Cannot verify multi-volume archives Cannot verify stdin/stdout archive Cannot write to %s Cannot write to compression program Child cannot fork Child died with signal %d%s Child returned status %d Conflicting archive format options Conflicting compression options Could not allocate memory for blocking factor %d Could not backspace archive file; it may be unreadable without -i Could not get current directory Could not get current directory: %s Could not re-position archive file Could not rewind archive file for verify Could only read %d of %ld bytes Cowardly refusing to create an empty archive Creating directory: Data differs Deleting non-header from archive Device numbers changed Directory %s has been renamed Directory %s is new Does not exist Error while closing %s Error while deleting %s Extracting contiguous files as regular files File does not exist File name %s%s too long File name %s/%s too long GNU features wanted on incompatible archive format Garbage command Generate data files for GNU tar test suite.
 Gid differs If a long option shows an argument as mandatory, then it is mandatory
for the equivalent short option also.

  -l, --file-length=LENGTH   LENGTH of generated file
  -p, --pattern=PATTERN      PATTERN is `default' or `zeros'
      --help                 display this help and exit
      --version              output version information and exit
 Invalid date format `%s' Invalid mode given on option Memory exhausted Missing file name after -C Mod time differs Mode differs Mode or device-type changed More than one threshold date Multiple archive files requires `-M' option No archive name given No longer a directory No new volume; exiting.
 No such file or directory Not a regular file Not linked to %s Obsolete option name replaced by --absolute-names Obsolete option name replaced by --backup Obsolete option name replaced by --block-number Obsolete option name replaced by --blocking-factor Obsolete option name replaced by --read-full-records Obsolete option name replaced by --touch Obsolete option, now implied by --blocking-factor Old option `%c' requires an argument. Options `-%s' and `-%s' both want standard input Options `-Aru' are incompatible with `-f -' Options `-[0-7][lmh]' not supported by *this* tar Premature end of file Read checkpoint %d Read error on %s Reading %s
 Record size must be a multiple of %d. Removing leading `/' from absolute links Removing leading `/' from absolute path names in the archive Renamed %s to %s Size differs Skipping to next header Symlink differs Symlinked %s to %s This does not look like a tar archive This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 This volume is out of sequence Too many errors, quitting Total bytes written:  Try `%s --help' for more information.
 Uid differs Unknown demangling command %s Unknown pattern `%s' Unknown system error Verify  Volume `%s' does not match `%s' WARNING: Archive is incomplete WARNING: Cannot close %s (%d, %d) WARNING: No volume header Write checkpoint %d Write to compression program short %d bytes You may not specify more than one `-Acdtrux' option exec/tcp: Service not available rmtd: Garbage command %c
 stdin stdout tar (child) tar (grandchild) Project-Id-Version: tar 1.12
POT-Creation-Date: 1998-02-19 21:41-0500
PO-Revision-Date: 1997-04-26 17:29+0200
Last-Translator: Rafa� Maszkowski <rzm@pdi.net>
Language-Team: Polish <pl@li.org>
MIME-Version: 1.0
Content-Type: text/plain; charset=ISO-8859-2
Content-Transfer-Encoding: 8-bit
 Przygotuj cz�� numer %d dla %s i naci�nij return 
Copyright (C) 1988, 92, 93, 94, 95, 96, 97 Free Software Foundation, Inc.
 
Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.
 
Parametry blok�w:
  -b, --blocking-factor=BLOKI    BLOKI x 512 bajt�w na rekord
      --record-size=ROZMIAR      ROZMIAR bajt�w na rekord, wielokrotno�� 512
  -i, --ignore-zeros             ignoruj wyzerowane bloki (oznacza EOF)
  -B, --read-full-records        podziel na bloki czytaj�c (dla pipe 4.2BSD)
 
Wyb�r i prze��czanie urz�dze�:
  -f, --file=ARCHIWUM            u�yj pliku lub urz�dzenia ARCHIWUM
      --force-local              plik archiwum lokalny, nawet z dwukropkiem
      --rsh-command=KOMENDA      u�yj KOMENDY zamiast rsh
  -[0-7][lmh]                    podaj nap�d i g�sto��
  -M, --multi-volume             tw�rz/wypisz/rozpakuj archiwum wielocz�ciowe
  -L, --tape-length=ILE          zmie� ta�m� po zapisaniu ILE x 1024 bajt�w
  -F, --info-script=PLIK         uruchom skrypt na ko�cu ta�my (w��cza -M)
      --new-volume-script=PLIK   to samo co -F PLIK
      --volno-file=PLIK          u�yj/uaktualnij numer cz�ci w PLIKu
 
Argumenty obowi�zkowe dla opcji d�ugich obowi�zuj� r�wnie� dla kr�tkich.
Podobnie je�eli argument jest podany jako opcjonalny.
 
Wyb�r plik�w lokalnych:
  -C, --directory KATALOG      przejd� do KATALOGu
  -T, --files-from=NAZWA       nazwy plik�w do roz/zapakowania s� w pliku NAZWA
      --null                   -T czyta nazwy zako�czone zerem, wy��cz -C
      --exclude=WZORZEC        wyklucz nazwy pasuj�ce do WZORCA
  -X, --exclude-from=PLIK      wyklucz nazwy pasuj�ce do wzorc�w w PLIKu
  -P, --absolute-names         nie usuwaj pocz�tkowego '/' z nazw plik�w
  -h, --dereference            zamiast plik�w pakuj symlinki na nie wskazuj�ce
      --no-recurse             nie wg��biaj si� automatycznie w katalogi
  -l, --one-file-system        archiwizuj�c pozosta� w jednym systemi plik�w
  -K, --starting-file=NAZWA    zacznij od pliku o tej NAZWie w archiwum
 
Przyrostek kopii zapasowej to `~', je�eli nie zosta� zmieniony przez --suffix
lub SIMPLE_BACKUP_SUFFIX.  Spos�b zarz�dzania wersjami mo�e by� zmieniony przez
--backup lub VERSION_CONTROL, mo�liwe warto�ci:

  t, numbered     zr�b numerowane kopie zapasowe
  nil, existing   numerowane je�eli takie ju� s�, w przeciwnym wypadku proste
  never, simple   proste kopie zapasowe
 
U�ycie: %s [OPCJA]...
 
U�ycie: %s [OPCJA]... [PLIK]...
 
Napisany przez Fran�oisa Pinarda <pinard@iro.umontreal.ca>.
 
Napisany przez Johna Gilmore'a i Jay'a Fenlasona.
   -N, --newer=DATA             zapisuj tylko pliki nowsze ni� DATA
      --newer-mtime            por�wnuj dat� i czas tylko dla zmienionych danych
      --after-date=DATA        to samo co -N
  (zrzut pami�ci)  ��cze do %s
  n [nazwa]  Podaj now� nazw� dla nast�pnej (i kolejnych) cz�ci
 q          Informacje o programie tar
 !          Uruchom shella
 ?          Wypisz t� list�
  nieznany typ pliku `%c'
 %d przy %d
 %s nie jest kontynuowany w tej cz�ci %s jest tym archiwum; nie zapisuj� %s: Nie mog� zmieni� czas�w dost�pu i modyfikacji %s: Nie mog�em utworzy� katalogu %s: Nie mog�em utworzy� pliku %s: Nie mog�em utworzy� ��cza symbolicznego do %s' %s: Nie mog�em do��czy� do %s' %s: Nie mog�em utworzy� fifo %s: Nie mog�em utworzy� pliku specjalnego %s: Nie mog� pisa� do pliku %s: Usuwam %s
 %s: B��d w czasie zamykania %s: Nie znalaz�em w archiwum %s: Nieznany typ pliku; plik zignorowany %s: Nie mog�em zrobi� kopii zapasowej tego pliku %s: jest niezmienione; nie zapisuj� ((proces potomny)) potok do stdin ((proces potomny)) potok do stdout (proces potomny) potok do stdin (proces potomny) potok do stdout (wnuczek) potok do stdin (wnuczek) potok do stdout --Zakodowane nazwy plik�w--
 --Nag��wek cz�ci--
 Dodane uprawnienia pisania i wykonywania do katalogu %s Niejednoznaczny wz�r `%s' Ilo�� (mam nadziej�) ostatecznie zapisana: %d.
 Etykieta archiwum nie pasuje do `%s' Archiwizuj� do stdin Archiwizuj� do stdout Na pocz�tku ta�my, teraz ko�cz� Pr�buj� odtworzy� ��cza symboliczne jako zwyk�e Nie mog� doda� katalogu %s Nie mog� doda� pliku %s Nie mog� przydzieli� miejsca na bufor Nie mog� przej�� do katalogu %s Nie mog� zmieni� katalogu na %s Nie mog� zamkn�� deskryptora %d Nie mog� zamkn�� pliku #%d Nie mog� wykona� %s Nie mog� uruchomi� shella %s Nie mog� wykona� zdalnego shella Nie mog� odtworzy� %s' -- plik jest kontynuowany z innej cz�ci Nie mog� otworzy� %s Nie mog� otworzy� archiwum %s Nie mog� otworzy� katalogu %s Nie mog� otworzy� pliku %s Nie mog� zamkn�� potoku Nie mog� prawied�owo powieli� %s Nie mog� czyta� %s Nie mog� przeczyta� potwiedzenia u�ytkownika Nie mog� czyta� z programu kompresuj�cego Nie mog� przeczyta� ��cza %s Nie mog� usun�� %s Nie mog� przemianowa� %s na %s Nie mog� ustawi� pozycji %ld w pliku %s Nie mog� uzyska� informacji (stat) o %s Nie mog� sprawdzi� stanu pliku %s Nie mog� utworzy� ��cza symbolicznego z %s do %s Nie mog� uaktualni� archiwum skompresowanego Nie mog� u�ywa� wielocze�ciowego archiwum skompresowanego Nie mog� zweryfikowa� archiwum skompresowanego Nie mog� zweryfikowa� archiwum wielocz�ciowego Nie mog� zweryfikowa� archiwum z/do stdin/stdout Nie mog� pisa� do %s Nie mog� pisa� do programu kompresuj�cego Proces potomny nie mo�e rozwidli� procesu Proces potomny zosta� zabity sygna�em %d%s Proces potomny zwr�ci� stan %d Niezgodne opcje formatu archiwum Niezgodne opcje kompresji Nie mog�em przydzieli� pami�ci dla blok�w %d Nie mog� si� cofn�� w pliku archiwum; mo�e nie by� czytelny bez -i Nie mog�em si� dosta� do bie��cego katalogu Nie mog�em si� dosta� do bie��cego katalogu: %s Nie mog� ponownie ustawi� pozycji w pliku archiwum Nie mog� przewin�� pliku archiwum dla weryfikacji Mog�em przeczyta� tylko %d z %ld bajt�w Tch�rzliwie odmawiam utworzenia pustego archiwum Tworz� katalog: Dane si� r�ni� Kasuj� z archiwum nie-nag��wek Numery urz�dze� zmienione Nazwa katalogu %s zosta�a zmieniona Katalog %s jest nowy Nie istnieje B��d w czasie zamykania %s B��d w czasie usuwania %s Odtwarzam pliki ci�g�e jako zwyk�e Plik nie istnieje Nazwa pliku %s%s jest za d�uga Nazwa pliku %s/%s jest za d�uga Pr�ba u�ycia rozszerze� GNU z niekompatybilnym formatem archiwum Bezsensowna komenda Utw�rz pliki danych do zestawu test�w tara GNU.
 Gid si� r�ni Argumenty obowi�zkowe dla opcji d�ugich obowiazuj� r�wnie� dla odpowiednich
kr�tkich.

  -l, --file-length=D�UGO��  D�UGO�� utworzonego pliku
  -p, --pattern=WZ�R         WZ�R: `default' (domy�lny) albo `zeros' (zera)
      --help                 wy�wietl ten opis i zako�cz
      --version              wy�wietl informacj� o wersji i zako�cz
 B��dny format daty `%s' B��dne uprawnienia podane w opcji Brak pami�ci Brakuj�ca nazwa pliku po -C Czas modyfikacji si� r�ni Uprawnienia si� r�ni� Zmieni�y si� uprawnienia albo typ urz�dzenia Wi�cej ni� jedna data graniczna Wiele plik�w archiwum wymaga opcji `-M' Nie podana nazwa archiwum Nie jest ju� katalogiem Brak nowej cz�ci; ko�cz�.
 Nie ma takiego pliku ani katalogu Nie jest zwyk�ym plikiem Nie do��czony do %s Przestarza�a nazwa opcji zamieniona na --absolute-names Przestarza�a nazwa opcji, zamieniona na --backup Przestarza�a nazwa opcji zamieniona na --block-number Przestarza�a opcja, wymieniona na --blocking-factor Przestarzala nazwa opcji zmieniona na --read-full-records Przestarza�a nazwa opcja zamieniona na --touch Przestarza�a opcja, teraz w��cza j� --blocking-factor Stara opcja `%c' wymaga argumentu. Opcje `-%s' i `-%s' obie chc� dost�pu do standardowego wej�cia Opce `-Aru s� niekompatybilne z `-f -' Opcje `-[0-7][lmh]' nie s� u�ywane w *tym* tarze Przedwczesny koniec pliku Czytam punkt kontrolny %d B��d czytania %s Czytam %s
 Rozmiar rekordu musi by� wielokrotno�ci� %d Usuwam pocz�tkowy `/' ze �cie�ek bezwzgl�dnych Usuwam pocz�tkowy `/' ze �cie�ek absolutnych w archiwum Przemianowa�em %s na %s Rozmiar si� r�ni Przeskakuj� do nast�pnego nag��wka ��cze symboliczne si� r�ni Do��czy�em symbolicznie %s do %s To nie wygl�da jak archiwum tar Ten program jest darmowy; warunki kopiowania s� opisane w �r�d�ach.
Autorzy nie daj� �ADNYCH gwarancji, w tym r�wnie� gwarancji PRZYDATNO�CI
DO SPRZEDA�Y LUB DO KONKRETNYCH CEL�W.
 Ta cz�� nie jest kolejn� Za du�o b��d�w, ko�cz� Ca�kowita zapisana ilo�� bajt�w:  Spr�buj `%s --help' �eby otrzyma� wi�cej informacji
 Uid si� r�ni Nieznana komenda odkodowuj�ca nazwy %s Nieznany wz�r `%s' Nieznany b��d systemu Sprawdzam  Cz�� `%s' nie pasuje do `%s' UWAGA: Archiwum jest niekompletne UWAGA: Nie mog� zamkn�� %s (%d, %d) UWAGA: Brak etykiety cz�ci Zapis punktu kontrolnego %d Zapis do programu kompresuj�cego skr�cony o %d bajt�w Nie mo�esz poda� wi�cej ni� jednej opcji `-Acdtrux' exec/tcp: Us�uga niedost�pna rmtd: Bezsensowna komenda %c
 stdin stdout tar (potomny) tar (wnuczek) 