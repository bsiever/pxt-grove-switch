# Platform

GitHub Co-Pilot in VS Code.  Agent mode with GPT-5.3-Codex

# Prompt

```
Hi -- I'd like to update this for the Sensirion SEN66 from the 55.  The structure should be updated:
* Files renamed as appropriate
* Configurations and descriptions updated as appropriate. 
* Then we should review code and update as needed.
```



# Grove DIP Switch conversion

Platform: Claude (Opus 5.5), linked to the local project folder.

```
The pxt-grove-switch directory contains an i2c driver for the sensirion sen 55 sensor, which is also
i2c based. Can you update/rewrite it to instead support the grove dip switches that you just
described in protocol.md? rename file, update settings in the .json, update the .cpp and .h files
with new blocks and comments.
```
