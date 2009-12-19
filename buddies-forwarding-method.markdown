---
layout: default
title: Buddies forwarding method
---

### Synchronization of remote buddy list

This page describes the method which is used to forward buddies from legacy network to XMPP user. We don't have to mind "to" or "from"
subscription types, because buddy doesn't need to see user's show and when user subscribes contact, we send subscribe request automatically

1. Libpurple creates new PurpleBuddy. Spectrum checks if this buddy is already in roster or not. If the buddy is not in roster, 
it's added to the queue. We can't send the buddy just when it's created, because there doesn't have to be nickname set in that time.

2. Spectrum checks repeatedly the queue whether there are some new buddies.

3. If the queue doesn't grow anymore, Spectrum sends buddies from the queue to the XMPP user. It must send buddies to the resource with highest 
priority. Spectrum should send Roster Item Exchange (RIE) stanza, if the resource supports it. Otherwise it sends subscribe stanzas.
Buddies' subscription is set to ASK and they are added to the database.

4. a) If the user responds with "subscribed" stanza, transport changes subscription to BOTH.

   b) If the user responds with "unsubscribed" stanza, transport changes subscription to NONE.
   
   c) If the user responds with "subscribe" (that's in case he wants to add contact we sent him in RIE), transport changes 
   subscription to BOTH and responds with "subscribed" stanza. Transport also sends "subscribe" stanza.

   d) **We have one problem here. We have to set subscription to NONE if user doesn't want to add buddies we sent him by RIE. 
   If we don't do it, buddies will be sent again on connect. Do you know how to do it?**

5. a) If the user sends "subscribe" request for buddy which is already in roster, Spectrum just reponds with "subscribed" stanza.

   b) If the user sends "subscribe" request for buddy which is not in roster, Spectrum forwards that request to legacy network and
   informs user about result by sending "subscribed" or "unsubscribed" stanza. Buddy is added to database in case he accepts authorization
   request.

   c) If the user sends "unsubscribe" or "unsubscribed" stanza for buddy which is already in roster, Spectrum reponds with "unsubscribed" stanza and
   sends "unsubscribe" stanza as well. Spectrum also removes that buddy from database.

### Forwarding lost subscription request 

In cases we've sent RIE stanza, but we haven't received "subscribe" stanza for some buddies from that stanza, we have to resend RIE. 
So the synchronization method described above is also repeated for all buddies with subscription ASK when user logins.