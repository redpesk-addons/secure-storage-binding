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
_AFT.setBeforeEach(function() print("~~~~~ Begin Test ~~~~~") end)
_AFT.setAfterEach(function() print("~~~~~ End Test ~~~~~") end)

_AFT.setBeforeAll(function() print("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++") return 0 end)
_AFT.setAfterAll(function() print("----------------------------------------------------------------------------------------") return 0 end)

local test_prefix="test_SecStorage"
local api="secstorage"

local verb='Read'
local test_name=test_prefix..'_'..api..'_'..verb..'_Error'..'_Read_empty'
_AFT.testVerbStatusError(test_name, api, verb, {key='noentry'}, 'None')

local verb='Write'
local test_name=test_prefix..'_'..api..'_'..verb
_AFT.testVerbStatusSuccess(test_name,api, verb, {key='name',value='IoT.bzh'} )

local verb='Write'
local test_name=test_prefix..'_'..api..'_Level1_'..verb
_AFT.testVerbStatusSuccess(test_name,api, verb, {key='/level1/name',value='level1_IoT.bzh'} )

local verb='Write'
local test_name=test_prefix..'_'..api..'_Level2_'..verb
_AFT.testVerbStatusSuccess(test_name,api, verb, {key='/level1/level2/name',value='level2_IoT.bzh'} )

local verb='Read'
local test_name=test_prefix..'_'..api..'_'..verb
_AFT.describe(test_name,function()
_AFT.assertVerbResponseEquals(api, verb, {key='/name'},{value='IoT.bzh'})
end)

local verb='Read'
local test_name=test_prefix..'_'..api..'_Level1_'..verb
_AFT.describe(test_name,function()
_AFT.assertVerbResponseEquals(api, verb, {key='/level1/name'},{value='level1_IoT.bzh'})
end)



local verb='Read'
local test_name=test_prefix..'_'..api..'_Level2_'..verb
_AFT.describe(test_name,function()
_AFT.assertVerbResponseEquals(api, verb, {key='/level1/level2/name'},{value='level2_IoT.bzh'})
end)



local verb='Write'
local test_name=test_prefix..'_'..api..'_Error_path'..verb
_AFT.testVerbStatusError(test_name,api, verb, {key='name/',value='error_IoT.bzh'} )



local verb='Read'
local test_name=test_prefix..'_'..api..'_'..verb..'_Error_'
_AFT.testVerbStatusError(test_name, api, verb, {key='/global/name'}, 'ronan' )



local verb='Delete'
local test_name=test_prefix..'_'..api..'_1_'..verb
_AFT.testVerbStatusSuccess(test_name,api, verb, {key='name'})



local verb='Delete'
local test_name=test_prefix..'_'..api..'_Level1_'..verb
_AFT.testVerbStatusSuccess(test_name,api, verb, {key='/level1/name'})



local verb='Delete'
local test_name=test_prefix..'_'..api..'_Level2_'..verb
_AFT.testVerbStatusSuccess(test_name,api, verb, {key='/level1/level2/name'})



local verb='Delete'
local test_name=test_prefix..'_'..api..'_2_time_'..verb
_AFT.testVerbStatusError(test_name,api, verb, {key='name'})



local verb='Delete'
local test_name=test_prefix..'_'..api..'_2_'..verb
_AFT.testVerbStatusError(test_name,api, verb, {key='/global'})


