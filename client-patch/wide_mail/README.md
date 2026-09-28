# Exact Zeny mail companion

Built into PNWallet64.dll and opened by its Mail button. Enter recipient character,
title, message and Zeny; request a quote, then explicitly confirm the recipient,
amount, fee and total debit. Editing the form invalidates its quote. Item-bearing
and received mail remain in native RODEX.

PZL1 v1 is Request636/Reply96. Its header is an exact copy of the staged server
contract. Recipient/title/body are bounded UTF-8 fields. All monetary arithmetic
uses signed 64-bit integers, with checked fee/total validation. One persistent
socket shares the wallet's authenticated-session observer without extra hooks;
replies must match the session nonce and character.

Send is issued once. A pending or uncertain response triggers Status polling,
never an automatic Send retry. Saved requires a successful durable status whose
send sequence, amount and total match the submitted request. A definitive refused
Send releases the form; an uncertain outcome keeps it locked until resolved.
Logout discards the session state and never resubmits the previous message.

Production Win32 tests cover an exact amount above 2^53, fees, changes during
confirmation, delayed acknowledgements, disconnect/status recovery and terminal
refusal. Loopback tests cover framing, fragmented replies and nonce/character
binding. Durable wallet/mail atomicity is a separate server fixture responsibility.
