#!/bin/sh

cp Build/FlowdockAPI/FlowdockAPI/libFlowdockAPI.so Build/FlowdockBot
mkdir Build/FlowdockBot/Handlers
cp Build/ScreencastLinkHandler/libScreencastLinkHandler.so Build/FlowdockBot/Handlers
cp Build/VSIDHandler/libVSIDHandler.so Build/FlowdockBot/Handlers
