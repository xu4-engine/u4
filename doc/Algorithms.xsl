<xsl:stylesheet version = '1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>
<xsl:strip-space elements="algorithm"/>

<!-- Main HTML Page -->
<xsl:template match="/">
    <HTML>
    <HEAD>
        <STYLE>
            * {
                font-family: Tahoma, Verdana, Arial;
                font-size: 9pt;
                vertical-align: top;
            }

            .section {
                color: white;
                font-size: 12pt;
                font-weight: bold;
                background: #888888;
                text-align: center;
            }

            .alg_header {
                color: white;
                font-weight: bold;
                background: #aaaaaa;
                text-align: center;
            }            
        </STYLE>
    </HEAD>
    <BODY style="margin: 0px; padding: 0px;">    

    <TABLE align="center" border="1" cellspacing="0" cellpadding="2" bordercolor="black">
    <TR><TD colspan="3" class="section" style="font-size: 15pt">Algorithms</TD></TR>    
    <TR><TD colspan="3" align="center">
        <TABLE border="0" align="center"><TR><TD><pre><xsl:apply-templates select="algorithms/intro"/></pre></TD></TR></TABLE>
    </TD></TR>
    <xsl:apply-templates select="algorithms/section"/>
    </TABLE>
        
    </BODY>
    </HTML>
</xsl:template>

<!-- Section header -->
<xsl:template match="section">    
    <TR><TD colspan="3" class="section"><xsl:value-of select="@name"/></TD></TR>
    <xsl:call-template name="alg_header"/>
    <xsl:apply-templates select="algorithm"/>
</xsl:template>

<!-- Algorithm header -->
<xsl:template name="alg_header">
    <TR class="alg_header"><TD>Description</TD><TD>Accuracy</TD><TD>Algorithm</TD></TR>
</xsl:template>

<!-- Algorithm -->
<xsl:template match="algorithm">    
    <TR>
        <TD><xsl:value-of select="@desc"/></TD>
        <TD><xsl:value-of select="@prec"/></TD>
        <TD><code>      
            <xsl:apply-templates select="line"/>
        </code></TD>
    </TR>
</xsl:template>

<!-- Line of code -->
<xsl:template match="line">
    <xsl:param name="indent" select="0"/>
    <div>
        <xsl:attribute name="style">padding-left: <xsl:value-of select="$indent*3"/>em; </xsl:attribute>        
        <xsl:value-of select="normalize-space(text())"/><br/>
        <xsl:apply-templates select="line">
            <xsl:with-param name="indent" select="1"/>
        </xsl:apply-templates>
    </div>
</xsl:template>

</xsl:stylesheet>