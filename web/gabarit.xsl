<?xml version="1.0" encoding="utf-8"?>
<stylesheet version="1.0"
            xmlns="http://www.w3.org/1999/XSL/Transform"
            xmlns:fp="http://pinard.progiciels-bpi.ca">
  <import href="/home/pinard/entretien/mes-sites/commun.xsl"/>
  <output method="html" encoding="UTF-8"/>
  <template match="/">
    <call-template name="gabarit-entretien">
      <with-param name="long-package-name" select="'Free pax utilities'"/>
      <with-param name="entries">

        <fp:section title="Documentation">
          <fp:entry text="NEWS" href="/NEWS.html"/>
        </fp:section>

        <fp:section title="Source files">
          <fp:entry text="Browse" href="http://github.com/pinard/paxutils"/>
          <fp:entry text="Download" href="/achives"/>
        </fp:section>

        <fp:section title="Development">
          <fp:entry text="TODO" href="/TODO.html"/>
          <fp:entry text="Contributors" href="/THANKS.html"/>
        </fp:section>

      </with-param>
    </call-template>
  </template>
</stylesheet>
