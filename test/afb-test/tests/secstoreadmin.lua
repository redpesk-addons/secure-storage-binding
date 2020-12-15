--[[
    Copyright (C) 2020 IoT.bzh Company
    Author: Ronan Le Martret <ronan.lemartret@iot.bzh>

    $RP_BEGIN_LICENSE$
    Commercial License Usage
     Licensees holding valid commercial IoT.bzh licenses may use this file in
     accordance with the commercial license agreement provided with the
     Software or, alternatively, in accordance with the terms contained in
     a written agreement between you and The IoT.bzh Company. For licensing terms
     and conditions see https://www.iot.bzh/terms-conditions. For further
     information use the contact form at https://www.iot.bzh/contact.

    GNU General Public License Usage
     Alternatively, this file may be used under the terms of the GNU General
     Public license version 3. This license is as published by the Free Software
     Foundation and appearing in the file LICENSE.GPLv3 included in the packaging
     of this file. Please review the following information to ensure the GNU
     General Public License requirements will be met
     https://www.gnu.org/licenses/gpl-3.0.html.
    $RP_END_LICENSE$


    NOTE: strict mode: every global variables should be prefixed by '_'
--]]

_AFT.setBeforeEach(function() print("~~~~~ Begin Test ~~~~~") end)
_AFT.setAfterEach(function() print("~~~~~ End Test ~~~~~") end)

_AFT.setBeforeAll(function() print("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++") return 0 end)
_AFT.setAfterAll(function() print("----------------------------------------------------------------------------------------") return 0 end)

local test_prefix= "test_SecStorageAdmin"
local api="secstoreadmin"

local verb='Write'
local test_name=test_prefix..'_'..api..'_1_'..verb
_AFT.testVerbStatusSuccess(test_name,api, verb, {key='/global/name',value='global_IoT.bzh'} )


local verb='Write'
local test_name=test_prefix..'_'..api..'_2_'..verb
_AFT.testVerbStatusSuccess(test_name,api, verb, {key='/global/level1/name',value='level1_global_IoT.bzh'} )


local verb='Write'
local test_name=test_prefix..'_'..api..'_3_'..verb
_AFT.testVerbStatusSuccess(test_name,api, verb, {key='/NoLabel/name',value='NoLabel_IoT.bzh'} )


local verb='Write'
local test_name=test_prefix..'_'..api..'_4_'..verb
_AFT.testVerbStatusSuccess(test_name,api, verb, {key='/NoLabel/level1/name',value='level1_NoLabel_IoT.bzh'} )

local verb='Write'
local test_name=test_prefix..'_'..api..'_5_'..verb
_AFT.testVerbStatusSuccess(test_name,api, verb, {key='/NoLabel/level1/2del',value='test2del'} )


local verb='Read'
local test_name=test_prefix..'_'..api..'_1_'..verb
_AFT.describe(test_name,function()
  _AFT.assertVerbResponseEquals(api, verb, {key='/NoLabel/level1/2del'}, {value='test2del'}) 
end)

local verb='Delete'
local test_name=test_prefix..'_'..api..'_1_'..verb
_AFT.testVerbStatusSuccess(test_name,api, verb, {key='/NoLabel/level1/2del'})


local verb='Write'
local test_name=test_prefix..'_'..api..'_5_'..verb
_AFT.testVerbStatusError(test_name,api, verb, {key='NotAllow/',value='None'} )


local verb='Write'
local test_name=test_prefix..'_'..api..'_6_'..verb
_AFT.testVerbStatusError(test_name,api, verb, {key='/NotAllow/',value='None'} )

local verb='CreateIter'
local test_name=test_prefix..'_'..api..'_1_'..verb
_AFT.testVerbStatusError(test_name,api, verb, {key='global/'} )

local verb='CreateIter'
local test_name=test_prefix..'_'..api..'_2_'..verb
_AFT.testVerbStatusError(test_name,api, verb, {key='/global'} )

local verb='CreateIter'
local test_name=test_prefix..'_'..api..'_3_'..verb
_AFT.testVerbStatusSuccess(test_name,api, verb, {key='/global/'} )

local verb='Next'
local test_name=test_prefix..'_'..api..'_1_'..verb
_AFT.testVerbStatusSuccess(test_name, api, verb, {} )

local verb='GetEntry'
local test_name=test_prefix..'_'..api..'_1_'..verb
_AFT.testVerbStatusSuccess(test_name, api, verb, {} )

local verb='DeleteIter'
local test_name=test_prefix..'_'..api..'_1_'..verb
_AFT.testVerbStatusSuccess(test_name, api, verb, {} )

local verb='GetTotalSpace'
local test_name=test_prefix..'_'..api..'_1_'..verb
_AFT.testVerbStatusSuccess(test_name, api, verb, {} )

local verb='CopyMetaTo'
local test_name=test_prefix..'_'..api..'_1_'..verb
_AFT.testVerbStatusSuccess(test_name, api, verb, {path='/tmp/test_copy.db'} )

verb='GetSize'
test_name=test_prefix..'_'..api..'_1_'..verb
_AFT.testVerbStatusSuccess(test_name,api, verb, {key='/NoLabel/'})
