--[[
    Copyright (C) 2020 "IoT.bzh"
    Author Ronan Le Martret <ronan.lemartret@iot.bzh>

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.


    NOTE: strict mode: every global variables should be prefixed by '_'
--]]


local test_prefix= "test_SecStorageAdmin"
local api="secstoreadmin"

local verb='Write'
local test_name=test_prefix..'_'..api..'_1_'..verb
_AFT.testVerbStatusSuccess(test_name,api, verb, {key='/global/name',value='global_IoT.bzh'} )
_AFT.setBefore(test_name,function() print("~~~~~ Begin "..test_name.." ~~~~~") end)
_AFT.setAfter(test_name,function() print("~~~~~ End "..test_name.." ~~~~~") end)

local verb='Write'
local test_name=test_prefix..'_'..api..'_2_'..verb
_AFT.testVerbStatusSuccess(test_name,api, verb, {key='/global/level1/name',value='level1_global_IoT.bzh'} )
_AFT.setBefore(test_name,function() print("~~~~~ Begin "..test_name.." ~~~~~") end)
_AFT.setAfter(test_name,function() print("~~~~~ End "..test_name.." ~~~~~") end)

local verb='Write'
local test_name=test_prefix..'_'..api..'_3_'..verb
_AFT.testVerbStatusSuccess(test_name,api, verb, {key='/NoLabel/name',value='NoLabel_IoT.bzh'} )
_AFT.setBefore(test_name,function() print("~~~~~ Begin "..test_name.." ~~~~~") end)
_AFT.setAfter(test_name,function() print("~~~~~ End "..test_name.." ~~~~~") end)

local verb='Write'
local test_name=test_prefix..'_'..api..'_4_'..verb
_AFT.testVerbStatusSuccess(test_name,api, verb, {key='/NoLabel/level1/name',value='level1_NoLabel_IoT.bzh'} )
_AFT.setBefore(test_name,function() print("~~~~~ Begin "..test_name.." ~~~~~") end)
_AFT.setAfter(test_name,function() print("~~~~~ End "..test_name.." ~~~~~") end)

local verb='Write'
local test_name=test_prefix..'_'..api..'_5_'..verb
_AFT.testVerbStatusError(test_name,api, verb, {key='NotAllow/',value='None'} )
_AFT.setBefore(test_name,function() print("~~~~~ Begin "..test_name.." ~~~~~") end)
_AFT.setAfter(test_name,function() print("~~~~~ End "..test_name.." ~~~~~") end)

local verb='Write'
local test_name=test_prefix..'_'..api..'_6_'..verb
_AFT.testVerbStatusError(test_name,api, verb, {key='/NotAllow/',value='None'} )
_AFT.setBefore(test_name,function() print("~~~~~ Begin "..test_name.." ~~~~~") end)
_AFT.setAfter(test_name,function() print("~~~~~ End "..test_name.." ~~~~~") end)

local verb='CreateIter'
local test_name=test_prefix..'_'..api..'_1_'..verb
_AFT.testVerbStatusError(test_name,api, verb, {key='global/'} )
_AFT.setBefore(test_name,function() print("~~~~~ Begin "..test_name.." ~~~~~") end)
_AFT.setAfter(test_name,function() print("~~~~~ End "..test_name.." ~~~~~") end)

local verb='CreateIter'
local test_name=test_prefix..'_'..api..'_2_'..verb
_AFT.testVerbStatusError(test_name,api, verb, {key='/global'} )
_AFT.setBefore(test_name,function() print("~~~~~ Begin "..test_name.." ~~~~~") end)
_AFT.setAfter(test_name,function() print("~~~~~ End "..test_name.." ~~~~~") end)

local verb='CreateIter'
local test_name=test_prefix..'_'..api..'_3_'..verb
_AFT.describe(test_name,function()
_AFT.assertVerbResponseEquals(api, verb, {key='/global/'},{iterator=1})
end)
_AFT.setBefore(test_name,function() print("~~~~~ Begin "..test_name.." ~~~~~") end)
_AFT.setAfter(test_name,function() print("~~~~~ End "..test_name.." ~~~~~") end)

