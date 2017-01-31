Attributes {
    NC_GLOBAL {
        OtherXML eml://ecoinformatics.org/eml-2.0.1 "
    
    <creator xmlns=\"\">
      <organizationName>Australian Commonwealth Scientific and Research Organization
	(CSIRO)</organizationName>
    </creator>
    <keywordSet xmlns=\"\">
      <keyword>remote sensing</keyword>
      <keyword>imagery</keyword>
      <keyword>spatial raster</keyword>
      <keyword>earth observation</keyword>
      <keyword>satellite data</keyword>
      
      <keyword>CZCS</keyword>
      <keyword>Chlorophyll</keyword>
    </keywordSet>
  ";
    }
    chlor {
        OtherXML eml://ecoinformatics.org/eml-2.0.1 "
      
      <attributeDefinition xmlns=\"\">chlor is level of chlorophyll a in mg m^-3 as measured by the
	satellite-borne CZCS sensor (From OtherXML, via NcML)</attributeDefinition>
      
      <horizontalAccuracy xmlns=\"\">
	<accuracyReport>These data have very little variation in the horzontal
	  dimensionality between pixels.</accuracyReport>
      </horizontalAccuracy>
      <verticalAccuracy xmlns=\"\">
	<accuracyReport>These data have very little variation in the vertical dimensionality
	  between pixels.</accuracyReport>
      </verticalAccuracy>
      <numberOfBands xmlns=\"\">1</numberOfBands>
      <rasterOrigin xmlns=\"\">Upper Left</rasterOrigin>
      <rows xmlns=\"\">2160</rows>
      <columns xmlns=\"\">4320</columns>
      <verticals xmlns=\"\">1</verticals>
      <cellGeometry xmlns=\"\">pixel</cellGeometry>
    ";
        time {
            OtherXML eml://ecoinformatics.org/eml-2.0.1 "
	<temporalCoverage xmlns=\"\">
	  <rangeOfDates>
	    <beginDate>
	      <calendarDate>1985-01-01</calendarDate>
	    </beginDate>
	    <endDate>
	      <calendarDate>1985-12-27</calendarDate>
	    </endDate>
	  </rangeOfDates>
	</temporalCoverage>
      ";
        }
    }
}
