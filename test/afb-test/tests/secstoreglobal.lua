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


local test_prefix= "test_SecStorage"
local api="secstoreglobal"

local verb='Write'
local test_name=test_prefix..'_'..api..'_'..verb
_AFT.testVerbStatusSuccess(test_name,api, verb, {key='name',value='Ronan'} )
_AFT.setBefore(test_name,function() print("~~~~~ Begin "..test_name.." ~~~~~") end)
_AFT.setAfter(test_name,function() print("~~~~~ End "..test_name.." ~~~~~") end)

verb='Read'
test_name=test_prefix..'_'..api..'_'..verb
_AFT.describe(test_name,function()
  _AFT.assertVerbResponseEquals(api, verb, {key='name'}, {value='Ronan'})
end)
_AFT.setBefore(test_name,function() print("~~~~~ Begin "..test_name.." ~~~~~") end)
_AFT.setAfter(test_name,function() print("~~~~~ End "..test_name.." ~~~~~") end)

verb='delete'
test_name=test_prefix..'_'..api..'_'..verb
_AFT.testVerbStatusSuccess(test_name,api, verb, {key='name'})
_AFT.setBefore(test_name,function() print("~~~~~ Begin "..test_name.." ~~~~~") end)
_AFT.setAfter(test_name,function() print("~~~~~ End "..test_name.." ~~~~~") end)

verb='Read'
test_name=test_prefix..'_'..api..'_'..verb..'_Error'
_AFT.testVerbStatusError(test_name, api, verb, {key='name'}, 'ronan' )
_AFT.setBefore(test_name,function() print("~~~~~ Begin "..test_name.." ~~~~~") end)
_AFT.setAfter(test_name,function() print("~~~~~ End "..test_name.." ~~~~~") end)
