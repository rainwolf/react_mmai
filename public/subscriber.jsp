<%@ page import="org.pente.game.*, org.pente.turnBased.*,
                 java.util.*, java.security.MessageDigest,
                 org.apache.commons.codec.binary.Hex,
                 org.pente.gameServer.client.web.*,
                 org.pente.gameServer.server.*,
                 org.pente.gameServer.core.*,
                 org.pente.kingOfTheHill.*"
         errorPage="../five00.jsp" %><% if (request.getAttribute("name") == null) {
    %><%="freeloader"%><%
   } else {
Resources resources = (Resources) application.getAttribute(
   Resources.class.getName());
DSGPlayerStorer dsgPlayerStorer= resources.getDsgPlayerStorer();
String nm = (String) request.getAttribute("name");
String name = nm;
DSGPlayerData dsgPlayerData = dsgPlayerStorer.loadPlayer(nm);
    %><%=dsgPlayerData.hasPlayerDonated()?"subscriber":"freeloader"%><%
} 
%>