|------------------------------------------------------------------------|
|- e3_Macro_Loot.inc v2.1 ( originally designed to work with e3.mac )	-|
|- Written by Killians of PEQ											-|
|------------------------------------------------------------------------|

|--------------------------------------------------------------------|
#event lootRelay "#1# tells you, 'loot #2#'"
SUB Event_lootRelay(line, ChatSender, looting)
	/if (!${checkEventArgs[${ChatSender},${line},UZ,""]}) /return
	/if (!${Defined[Auto_Loot]}) /declare Auto_Loot bool outer TRUE
	/if (${Select[${looting},on]}) {
		/varset Auto_Loot TRUE
		/r Looting Enabled
		/if (${Me.CombatState.NotEqual[Combat]} || ${combatLooting}) /call loot_Area

	} else /if (${Select[${looting},off]}) {
		/varset Auto_Loot FALSE
		/r Looting Disabled
	} else (${Select[${looting},xp]}) {
		/if (!${Defined[Xp_Loot]}) {
			/declare Xp_Loot bool outer True
			/r Will auto loot on XP!
		} else {
			/varset Xp_Loot !${Xp_Loot}
			/if (${Xp_loot}) {
				/r Will auto loot on XP!
			} else /r Not auto looting on XP!
		}
	}
/return

|---------------------------------------------------------------|
|- Loot on command if I am looter. Used if autoloot have missed some  -|
|- Will loot in combat even if combatLooting is false                 -|
#event LootIt "<#1#> LootIt#*#"
SUB EVENT_LootIt((line, ChatSender)
	/if (!${checkEventArgs[${ChatSender},${line},UZ,""]}) /return
	/if (${Auto_Loot}) {
		/if (${Me.CombatState.NotEqual[Combat]} || ${combatLooting}) {
			/call loot_Area
		}
	}
/RETURN

|-------------------------------------------------------------------|
|- Automatically engage looting functions when a mob dies.         -|
#event YourKill "You have gain experience#*#"
#event YourKill "You have gain #*# experience#*#"
#event YourKill "You have gain party experience#*#"
#event YourKill "You have gained #*# party experience#*#"
#event YourKill "You have gained raid experience#*#"
#event YourKill "You have gained #*# raid experience#*#"
SUB Event_YourKill
	/if (${Defined[Xp_Loot]} && ${Xp_Loot}) {
		/if (${Me.CombatState.NotEqual[Combat]} || ${combatLooting}) {
			/call loot_Area
		}
	}
/RETURN

|----------------------------------------------------------------|
|- Adds corpses that are not yours to the looted corpse list.	  -|
#EVENT NotYourKill "Someone is already looting that corpse."
#EVENT NotYourKill "You may not loot this corpse at this time."
#EVENT NotYourKill "You are too far away to loot that corpse."
SUB EVENT_NotYourKill
	/varset canLootCorpse FALSE
/RETURN


|----------------------------------------------------------------------------|
|- Moves to the closest corpses and opens to loot window.					-|
|- Just because the game is so fucked up with targeting corpses.			-|
#EVENT openClosestCorpse "<#1#> Open Closest Corpse#*#"
SUB EVENT_openClosestCorpse(line, ChatSender)
	/if (!${checkEventArgs[${ChatSender},${line},UZ,""]}) /return
	/if (!${c_eventArgChatSender.Equal[${Me.CleanName}]}) /return
	
	/declare ClosestCorpse        int local
	/declare ClosestCorpseDist    int local

	/varset ClosestCorpse ${NearestSpawn[1,npc corpse zradius 50 radius ${seek_Radius} "_"].ID}
	/if (${ClosestCorpse} > 0) {
		/if (${Bool[${Spawn[id ${ClosestCorpse}]}]}) {
			/varset ClosestCorpseDist ${Math.Distance[${Spawn[id ${ClosestCorpse}].Y},${Spawn[id ${ClosestCorpse}].X}]}
			/if (${Target.ID}!=${ClosestCorpse}) /call TrueTarget ${ClosestCorpse}
			/delay 2s ${Target.ID}==${ClosestCorpse}
			/if (${Target.Distance}>16 && ${Target.DistanceZ} < 80) {
			  /call MoveToLoc ${Target.Y} ${Target.X} 20 12
			}
			/loot
		}
	}
/RETURN

|----------------------------------------------------------------------------|
|- Moves to corpses and begins looting, reads Loot_Ini for loot handles.	-|
SUB loot_Area
	/squelch /turbo 100
	/declare startX int ${Me.X}
	/declare startY int ${Me.Y}
	/squelch /hidecor looted
	/if (${Twist.Twisting}) /call pauseTwist
	/delay 5s !${Me.Casting.ID}
	/if (${NetAdvPath.Following}) /squelch /netfollow off
	/if (${Stick.Active}) /squelch /stick off

	/declare ClosestCorpse        int local
	/declare ClosestCorpseDist    int local
	/declare CorpseList         string local
	/declare CorpseList_Looted  string local
	/declare i int local
	/declare c int local
	/declare NumCorpses int local ${SpawnCount[npc corpse zradius 50 radius ${seek_Radius} "_"]}
	/declare CorpsesLeft int local ${NumCorpses}
	/varset cantLootCorpseCount 0
	|/doevents flush YourKill
	/for i 1 to ${NumCorpses}
		/varset ClosestCorpse ${NearestSpawn[${i},npc corpse zradius 50 radius ${seek_Radius} "_"].ID}
		/varset CorpseList ${CorpseList}${ClosestCorpse}|
	/next i

	/for i 1 to ${NumCorpses}
		|iterate list of corpses
		/if (!${CorpsesLeft}) /return
		|get closest corpse from current position
		/varset ClosestCorpse ${CorpseList.Arg[1,|]}
		/varset ClosestCorpseDist ${Math.Distance[${Spawn[id ${ClosestCorpse}].Y},${Spawn[id ${ClosestCorpse}].X}]}
		/if (!${Bool[${Spawn[${ClosestCorpse}]}]}) /goto :skipCorpse

		/if (${Target.ID}!=${ClosestCorpse}) /call TrueTarget ${ClosestCorpse}
		/delay 2s ${Target.ID}==${ClosestCorpse}
		/if (!${Target.ID}) {
			/goto :skipCorpse
		}
		/if (${Target.Distance}>16 && ${Target.DistanceZ} < 80) {
			/call MoveToLoc ${Target.Y} ${Target.X} 20 12
		}
		/if (${Target.Distance}<=20 &&  ${Target.DistanceZ} < 40) {
			/call lootCorpse
		} else {
			/echo Corpse ${ClosestCorpse} is > ${Target.Distance}|${Target.DistanceZ} distance, skipping
		}
		:skipCorpse
		/varset CorpseList ${CorpseList.Replace[${ClosestCorpse}|,]}
		/varcalc CorpsesLeft ${CorpsesLeft}-1
		|exits loop if autoloot was turned off
		/doevents lootRelay
	/if (${Auto_Loot}) /next i
	|if all corpses in radius cant be looted, dont try again until a new kill occurs
	/if (${cantLootCorpseCount}>=${SpawnCount[npc corpse zradius 50 radius ${seek_Radius} "_"]}) /doevents flush YourKill

	/if (${Following}) {
		/call AcquireFollow
	} else {
		/call MoveToLoc ${startY} ${startX} 15 20
	}
    /if (${Defined[returnTwist]} && ${returnTwist}) /call unpauseTwist
	
	/squelch /turbo ${classTurbo}
/return

|-------------------------------------------------------------------------------------------------
SUB lootCorpse
	/declare i int local
	/declare j int local
	/declare itemName string local
	/declare itemSlot int local
	/varset canLootCorpse TRUE
	/loot
	/delay 5 ${Window[LootWnd].Open}
	/doevents NotYourKill
	/delay 1s ${Corpse.Items} || !${canLootCorpse}
	/declare numItems int local ${Corpse.Items}

	/if (!${canLootCorpse}) {
		/varcalc cantLootCorpseCount ${cantLootCorpseCount}+1
		/goto :skipLoot
	}
	/if (!${Bool[${Corpse.Items}]}) {
		/goto :skipLoot
	}

	|must use numItems, ${Corpse.Items} recalcs every iteration
	/for i 1 to ${numItems}
		/if (!${Me.FreeInventory}) {
			/if (!${fullInventory_Alert}) {
				/beep
				/varset fullInventory_Alert TRUE
				/docommand ${ChatToggle} My inventory is full!  I will continue to link items on corpses, but cannot loot anything else.
			}
		}

		/delay 1s ${Corpse.Item[${i}].ID}
		/call get_lootSetting ${i} 0

		/if (${lootSetting.Find[Skip]}) /goto :skipItem
		/varset itemName ${Corpse.Item[${i}].Name}

		| Destroy the item
		/if (${lootSetting.Find[Destroy]}) {
			/echo [Loot]: Destroying [${Corpse.Item[${i}].Name}]
			/if (${Corpse.Item[${i}].Lore} && ${FindItemCount[=${Corpse.Item[${i}].Name}]}) /goto :skip_Loot
			/call loot_Handle ${i} destroy
			| Keep the item
		} else /if (${lootSetting.Find[Keep]}) {
			| Check for a max stock
			/call argueString keep| "${lootSetting}"
			/if (${Bool[${c_argueString}]} && ${Int[${c_argueString}]} < ${Math.Calc[${FindItemCount[${Corpse.Item[${i}].Name}]} + 1].Int}) {
				/echo [Loot]: Fully stocked on [${Corpse.Item[${i}].Name}].
				/goto :skipItem
			}
			| If the item is lore, make sure I don't have one.  If I do, skip it.
			/if (${Corpse.Item[${i}].Lore}) {
				/if (${FindItemCount[=${Corpse.Item[${i}].Name}]} || ${FindItemBankCount[=${Corpse.Item[${i}].Name}]}) {
				/echo [Loot]: [${Corpse.Item[${i}].Name}] is lore, and I already have one${If[${FindItemBankCount[=${Corpse.Item[${i}].Name}]}, in the bank,]}.
				/goto :skipItem
			}
		}
		| Check that I have an inventory space large enough for the item
		| If
		| 1) No open slots
		| 2) The item on the corpse is stackable
		| 3) I have one of the stackable items in my inventory already
		| Then check to see if there is room within the stack to loot it.
		/if (${Me.FreeInventory}) {
		} else /if (!${Me.LargestFreeInventory} && ${Corpse.Item[${i}].Stackable} && ${Bool[${FindItem[=${itemName}]}]}) {
			/if (!${Bool[${FindItem[=${itemName}].ItemSlot}]}) /goto :skipItem
			/varset itemSlot ${FindItem[=${itemName}].ItemSlot}

			/if (${Bool[${Me.Inventory[${itemSlot}].Items}]}) {
				/for j 1 to 10
					/if (${Me.Inventory[${itemSlot}].Item[${j}].Name.Equal[${itemName}]}) {
						/if (!${Me.Inventory[${itemSlot}].Item[${j}].FreeStack} ) {
						/goto :skipItem
					}
				}
				/next j
			} else {
				/goto :skipItem
			}
		} else /if (${Me.LargestFreeInventory} < ${Corpse.Item[${i}].Size}) {
			/echo [Loot]: I don't have a free inventory space large enough to hold [${itemName}].
			/goto :skipItem
		} else /if (!${Me.FreeInventory}) {
			/goto :skipItem
		}
		/echo [Loot]: Looting [${Corpse.Item[${i}].Name}]
		/call loot_Handle ${i} keep
		/delay 1
		}
:skipItem

	/next i
	/delay 1
	|looting done, if items leftover link them
	/if (${Corpse.Items}) {
		/keypress /
		/delay 1
		/call Type "${linkChannel} ${Target.CleanName} ${Target.ID}-"
		/delay 1
		/notify LootWnd BroadcastButton leftmouseup
		/delay 1
		/keypress enter chat
	}
:skipLoot
	/if (${Corpse.Open}) /notify LootWnd DoneButton leftmouseup
	/delay 1s !${Window[LootWnd].Open}
/return

|--------------------------------------------------------------------------|
SUB get_ItemValue(intValue)
	/declare itemValue string local
	/declare vl int local
	/declare vlp int local
	/declare vlg int local
	/declare vls int local
	/declare vlps string local
	/declare vlgs string local
	/declare vlss string local
	/declare vlcs string local

	/varset itemValue ${intValue}

	/varset vl ${itemValue.Length}
	/varset vlp ${Math.Calc[${vl} - 3].Int}
	/varset vlg ${Math.Calc[${vl} - 2].Int}
	/varset vls ${Math.Calc[${vl} - 1].Int}
	
	/If (${vlp}>0) {
		/varset vlps ${itemValue.Left[${vlp}]}
	} else /varset vlps

	/If (${vlg}>0) {
		/varset vlgs ${itemValue.Mid[${vlg},1]}
	} else /varset vlgs

	/If (${vls}>0) {
		/varset vlss ${itemValue.Mid[${vls},1]}
	} else /varset vlss

	/varset vlcs ${itemValue.Right[1]}
	
	/varset c_itemValue ${If[${vlp}>0 && ${Bool[${vlps}]},${vlps}p,]}${If[${vlg}>0 && ${Bool[${vlgs}]},${vlgs}g,]}${If[${vls}>0 && ${Bool[${vlss}]},${vlss}s,]}${If[${Bool[${vlcs}]},${vlcs}c,]}
		
/return

|--------------------------------------------------------------------------|
SUB get_lootSetting(invSlot, itemSlot)
  /if (!${Bool[${Corpse.Items}]} && !${Window[MerchantWnd]}) /return
	/declare itemName string local
	/declare iniEntryVariables string local

	/varset lootSetting
	| If the item is not in a bag
	/if (!${Bool[${itemSlot}]}) {
		| Reset itemName if it contains a ':'.  ':'s cause errors when reading from the ini, because they act as a delimiter, just like '='s
		/varset itemName ${Corpse.Item[${invSlot}].Name}
		/if (${itemName.Find[:]}) /varset itemName ${itemName.Replace[:,;]}
		| This seems to crash Macroquest2 !!!!!
		|/if (${itemName.Find[,]}) /varset itemName ${itemName.Replace[",",.]}
		| Set item value
		/call get_itemValue ${Corpse.Item[${invSlot}].Value}
		
		| Set ini variables like stack size, (C), (ND) etc.
		/varset iniEntryVariables ${If[${Corpse.Item[${invSlot}].Stackable},(${Corpse.Item[${invSlot}].StackSize}),]}${If[${Corpse.Item[${invSlot}].NoDrop},(ND),]}${If[${Corpse.Item[${invSlot}].Lore},(L),]}${If[${Corpse.Item[${invSlot}].Container},(C),]}

		| Check for a Loot_Ini entry
		/if (!${Ini[${Loot_Ini},${itemName.Left[1]},${itemName} ${c_itemValue}${iniEntryVariables}].Length}) {
			/varset lootSetting ${If[${Corpse.Item[${invSlot}].Container},Container,${If[${Corpse.Item[${invSlot}].NoDrop},Skip,Keep${If[${Corpse.Item[${invSlot}].Stackable},|${Corpse.Item[${invSlot}].StackSize},]}]}]}
			/call WriteToIni "${Loot_Ini},${itemName.Left[1]},${itemName} ${c_itemValue}${iniEntryVariables}" ${lootSetting}
			/docommand ${ChatToggle} Loot Setting Added: ${itemName} ${c_itemValue}${iniEntryVariables}=${lootSetting}
		} else {
			/varset lootSetting ${Ini[${Loot_Ini},${itemName.Left[1]},${itemName} ${c_itemValue}${iniEntryVariables}]}
		}
	| If the item is in a bag
	} else {
		| Reset itemName if it contains a ':'.  ':'s cause errors when reading from the ini, because they act as a delimiter, just like '='s
		/varset itemName ${Me.Inventory[${invSlot}].Item[${itemSlot}].Name}
		/if (${itemName.Find[:]}) /varset itemName ${itemName.Replace[:,;]}
		| Set item value
		/call get_itemValue ${Me.Inventory[${invSlot}].Item[${itemSlot}].Value}
		
		| Set ini variables like stack size, (C), (ND) etc.
		/varset iniEntryVariables ${If[${Me.Inventory[${invSlot}].Item[${itemSlot}].Stackable},(${Me.Inventory[${invSlot}].Item[${itemSlot}].StackSize}),]}${If[${Me.Inventory[${invSlot}].Item[${itemSlot}].NoDrop},(ND),]}${If[${Me.Inventory[${invSlot}].Item[${itemSlot}].Lore},(L),]}${If[${Me.Inventory[${invSlot}].Item[${itemSlot}].Container},(C),]}

		| Check for a Loot_Ini entry
		/if (!${Ini[${Loot_Ini},${itemName.Left[1]},${itemName} ${c_itemValue}${iniEntryVariables}].Length}) {
			/varset lootSetting ${If[${Corpse.Item[${invSlot}].Container},Container,${If[${Corpse.Item[${invSlot}].NoDrop},Skip,Keep${If[${Corpse.Item[${invSlot}].Stackable},|${Corpse.Item[${invSlot}].StackSize},]}]}]}
			/call WriteToIni "${Loot_Ini},${itemName.Left[1]},${itemName} ${c_itemValue}${iniEntryVariables}" ${lootSetting}
			/docommand ${ChatToggle} Loot Setting Added: ${itemName} ${c_itemValue}${iniEntryVariables}=${lootSetting}
		} else {
			/varset lootSetting ${Ini[${Loot_Ini},${itemName.Left[1]},${itemName} ${c_itemValue}${iniEntryVariables}]}
		}
	}
/RETURN

|--------------------------------------------------------------------|
|- Loots items mobs you have killed, to destroy or autoinventory.   -|
|--------------------------------------------------------------------|
|- @param slotNum: The slot number you are looting from.			-|
|- @param handle: What to do with the item once you loot it.		-|
|--------------------------------------------------------------------|
SUB loot_Handle(int slotNum, handle)
	/declare lootTimer timer local 3s
	:retry_Loot
	| Try to loot the specified item from the corpse.
	/itemnotify loot${slotNum} leftmouseup
	/delay 1s ${Cursor.ID} || ${Window[ConfirmationDialogBox].Open} || ${Window[QuantityWnd].Open}
	| If the item has been looted, decide what to do with it.
	/if (${Cursor.ID}) {
		| Destroy the item.
		/if (${handle.Equal[destroy]}) {
			:destroy_Item
			/destroy
			| If the destroy attempt was unsuccessful, try again.	
			/if (${Cursor.ID} && ${lootTimer}) /goto :destroy_Item
		| Else, keep the item.
		} else {
			/call ClearCursor
		}
	| Else, if loot attempt was unsuccessful, try again.		
	} else {
		| Click yes on confirmation box.
		/if (${Window[ConfirmationDialogBox].Open}) {
			/notify ConfirmationDialogBox Yes_Button leftmouseup
		} else /if (${Window[QuantityWnd].Open}) {
			/notify QuantityWnd QTYW_Accept_Button leftmouseup
		}
		/if (${lootTimer}) /goto :retry_Loot
	}
/RETURN


|--------------------------------------------------------------------|
|- Check Loot Setting
#EVENT checkLootCursor "<#1#> Check Loot Setting Cursor"
#EVENT checkLootCursor "[MQ2] Check Loot Setting Cursor"
SUB EVENT_checkLootCursor(line, ChatSender)
	/if (${ChatSender.Equal[${Me.CleanName}]} || !${Defined[ChatSender]}) {
		/if (!${Cursor.ID}) {
			/echo Need an item on the cursor to check loot setting
		} else {
			/declare itemName string local ${Cursor.Name}
			/if (${itemName.Find[:]}) /varset itemName ${itemName.Replace[:,;]}
			
			| Set item value
			/call get_itemValue ${Cursor.Value}
			
			| Set ini variables like stack size, (C), (ND) etc.
			/declare iniEntryVariables string local ${If[${Cursor.Stackable},(${Cursor.StackSize}),]}${If[${Cursor.NoDrop},(ND),]}${If[${Cursor.Lore},(L),]}${If[${Cursor.Container},(C),]}
			
			/if (${Ini[${Loot_Ini},${itemName.Left[1]},${itemName} ${c_itemValue}${iniEntryVariables}].Length}) {
				/echo Loot setting for ${Cursor} is ${Ini[${Loot_Ini},${itemName.Left[1]},${itemName} ${c_itemValue}${iniEntryVariables}]}
			} else {
				/echo No loot setting for ${Cursor}
			}
		}
	}
/RETURN


|--------------------------------------------------------------------|
|- Update Loot Setting
#EVENT updateLootCursor "<#1#> Set Loot Setting Cursor /#2#"
#EVENT updateLootCursor "[MQ2] Set Loot Setting Cursor /#2#"
SUB EVENT_updateLootCursor(line, ChatSender, iniSetting)
	/if (${ChatSender.Equal[${Me.CleanName}]} || !${Defined[ChatSender]}) {
		/if (!${Cursor.ID}) {
			/echo Need an item on the cursor to check loot setting
		} else {
			/declare itemName string local ${Cursor.Name}
			/if (${itemName.Find[:]}) /varset itemName ${itemName.Replace[:,;]}
			
			| Set item value
			/call get_itemValue ${Cursor.Value}
			
			| Set ini variables like stack size, (C), (ND) etc.
			/declare iniEntryVariables string local ${If[${Cursor.Stackable},(${Cursor.StackSize}),]}${If[${Cursor.NoDrop},(ND),]}${If[${Cursor.Lore},(L),]}${If[${Cursor.Container},(C),]}
			
			/if (${Ini[${Loot_Ini},${itemName.Left[1]},${itemName} ${c_itemValue}${iniEntryVariables}].Length}) {
				/declare oldSetting string local ${Ini[${Loot_Ini},${itemName.Left[1]},${itemName} ${c_itemValue}${iniEntryVariables}]}
				/call WriteToIni "${Loot_Ini},${itemName.Left[1]},${itemName} ${c_itemValue}${iniEntryVariables}" "${iniSetting}" TRUE
				/echo Loot setting for ${Cursor} changed from ${oldSetting} to ${iniSetting}
			} else {
				/call WriteToIni "${Loot_Ini},${itemName.Left[1]},${itemName} ${c_itemValue}${iniEntryVariables}" "${iniSetting}" TRUE
				/echo Loot setting for ${Cursor} added as ${iniSetting}
			}
		}
	}
/RETURN


|--------------------------------------------------------------------|
|- Check Loot Setting (OLD!!! DOESN'T WORK)
#EVENT checkLootSetting "<#*#> Check Loot Setting for #1#"
#EVENT checkLootSetting "[MQ2] Check Loot Setting for #1#"
SUB EVENT_checkLootSetting(line, itemName)
	/declare itemToCheck string local ${itemName}
	/if (${itemToCheck.Find[:]}) /varset itemToCheck ${itemToCheck.Replace[:,;]}
	/declare i int local 1
	
	:check_Loot_Ini
	/if (!${Ini[${Loot_Ini},${itemToCheck.Left[1]}].Arg[${i},|].Arg[1,${If[${Ini[${Loot_Ini},${itemToCheck.Left[1]}].Arg[${i},|].Find[~]},~,(]}].Equal[${itemToCheck}]}) {
		/varcalc i ${i} + 1
		/if (${Ini[${Loot_Ini},${itemToCheck.Left[1]}].Arg[${i},|].Length}) {
			/goto :check_Loot_Ini
		} else {
			/echo No entry found.
		}
	} else {
		/echo >>> [${itemName}=${Ini[${Loot_Ini},${itemToCheck.Left[1]},${Ini[${Loot_Ini},${itemToCheck.Left[1]}].Arg[${i},|]}]}] <<<
	}
/RETURN

|--------------------------------------------------------------------|
|- Update Loot Setting (OLD!!! DOESN'T WORK)
#EVENT updateLootSetting "<#1#> Set Loot Setting for #2# /#3#"
#EVENT updateLootSetting "[MQ2] Set Loot Setting for #2# /#3#"
SUB EVENT_updateLootSetting(line, ChatSender, itemName, iniSetting)
	/if (${ChatSender.Equal[${Me.CleanName}]} || !${Defined[ChatSender]}) {
		/declare itemToCheck string local ${itemName}
		/if (${itemToCheck.Find[:]}) /varset itemToCheck ${itemToCheck.Replace[:,;]}
		/declare i int local 1
		
		:check_Loot_Ini
		/if (!${Ini[${Loot_Ini},${itemToCheck.Left[1]}].Arg[${i},|].Arg[1,${If[${Ini[${Loot_Ini},${itemToCheck.Left[1]}].Arg[${i},|].Find[~]},~,(]}].Equal[${itemToCheck}]}) {
			/varcalc i ${i} + 1
			/if (${Ini[${Loot_Ini},${itemToCheck.Left[1]}].Arg[${i},|].Length}) {
				/goto :check_Loot_Ini
			} else {
				/echo No entry found.
			}
		} else {
			/call WriteToIni "${Loot_Ini},${itemToCheck.Left[1]},${Ini[${Loot_Ini},${itemToCheck.Left[1]}].Arg[${i},|]}" "${iniSetting}" 1
			/echo >>> Loot_Ini entry for [${itemName}] has been changed to [${Ini[${Loot_Ini},${itemToCheck.Left[1]},${Ini[${Loot_Ini},${itemToCheck.Left[1]}].Arg[${i},|]}]}]. <<<
		}
	}
/RETURN

|--------------------------------------------------------------------|
| Make a specific bot move to a corpse and loot a specific item.
| 
|  corpseID : ID of the corpse to loot from
|  charName : Name of the bot to loot the item
|  itemName : Partial name of the item to loot
|  equipSlot : Optional. name of slot to equip the item in.
|
#EVENT lootItem "<#1#> Loot Item #2# #3# #4# #5#"
#EVENT lootItem "<#1#> Loot Item #2# #3# #4#"
SUB EVENT_lootItem(line, ChatSender, corpseID, charName, itemName, equipSlot)
	/if (!${Me.CleanName.Equal[${charName}]}) /RETURN

	/if (!${Me.FreeInventory}) {
		/docommand ${ChatToggle} My inventory is already full
		/RETURN
	}

	/if (${Twist.Twisting}) /call pauseTwist
	/delay 10s !${Me.Casting.ID}
	/if (${NetAdvPath.Following}) /squelch /netfollow off
	/if (${Stick.Active}) /squelch /stick off

    |check the distance
	/if (${Bool[${Spawn[ID ${corpseID}].ID}]} && (${Spawn[ID ${corpseID}].Distance.Int} < ${seek_Radius})) {
		|move to the corpse
		/call TrueTarget ${corpseID}
		/target ID ${corpseID}
		/delay 7 ${Target.ID}==${corpseID}
		/if (${Target.ID}!=${corpseID}) {
			/docommand ${chatToggle} Can't target corpse ${corpseID}
			/goto :skip_loot
		}
		/if (${Target.Distance}>16 && ${Target.DistanceZ} < 80) {
			/call MoveToLoc ${Target.Y} ${Target.X} 20 12
		}
		/if (${Target.Distance}<=20 &&  ${Target.DistanceZ} < 40) {
			/loot
			/delay 2s ${Corpse.Open}
			| check if window open
			/if (${Corpse.Open}) {
				|make sure the loot window has been populated
			    /delay 1s ${Corpse.Item[${itemName}].ID}
				| is there such an item on the corpse?
				/if (${Corpse.Item[${itemName}].ID}) {
					|Gets the full item name
					/declare fullItemName local ${Corpse.Item[${itemName}]}
					/if (${Corpse.Item[=${fullItemName}].Lore} && ${FindItemCount[${fullItemName}]}) {
						/docommand ${ChatToggle} ${fullItemName} is Lore and I already have one.
						/goto :skip_loot
					}
					/docommand ${ChatToggle} Looting ${fullItemName}
					/echo Looting ${fullItemName} from slot ${Corpse.Item[=${fullItemName}].InvSlot}
					/declare lootTimer timer local 3s

					:retryLoot
					/itemnotify ${Corpse.Item[=${fullItemName}].InvSlot} leftmouseup
					/delay 2s ${Cursor.ID} || ${Window[ConfirmationDialogBox].Open} || ${Window[QuantityWnd].Open}
					/if (${Cursor.ID}) {
						/call ClearCursor
					} else {
						/if (${Window[ConfirmationDialogBox].Open}) {
							/notify ConfirmationDialogBox CD_Yes_Button leftmouseup
						} else /if (${Window[QuantityWnd].Open}) {
							/notify QuantityWnd QTYW_Accept_Button leftmouseup
						}
						/if (${lootTimer}) /goto :retryLoot
					}

					/notify LootWnd LW_DoneButton leftmouseup
					|/delay 1s !${Window[LootWnd].Open}
					/if (${Cursor.ID}) {
						/beep
						/docommand ${ChatToggle} I am stuck with ${Cursor.Name} on my cursor. Ending macro!
						/endmacro
					}
					/if (${Select[${equipSlot},charm,leftear,head,face,rightear,neck,shoulder,arms,back,leftwrist,rightwrist,ranged,hands,mainhand,offhand,leftfinger,rightfinger,chest,legs,feet,waist,powersource,ammo]}) {
						/call SwapItem "${fullItemName}" ${equipSlot}
						/docommand ${ChatToggle} Equipping ${fullItemName} to ${equipSlot}
					}
				} else {
					/docommand ${ChatToggle} Can not find an item matching "${itemName}" on this corpse
				}
			} else {
				/docommand ${ChatToggle} Can't loot the corpse
			}
		} else {
			/docommand ${ChatToggle} I can't reach the corpse
		}
	} else {
		/docommand ${ChatToggle} Corpse is too far away, get closer
	}

:skip_loot	
	/if (${Following}) {
		/call AcquireFollow
	}
    /if (${Defined[returnTwist]} && ${returnTwist}) /call unpauseTwist
/RETURN

|--------------------------------------------------------------------|
SUB loot_Setup
	/call iniToVarV "${advSettings_Ini},Debug,Debug Loot (On/Off)" Debug_Loot bool outer
	/if (!${Ini[${MacroData_Ini},File Paths,Loot Settings File].Length}) /call WriteToIni "${MacroData_Ini},File Paths,Loot Settings File" "e3 Macro Inis\Loot Settings.ini" 1
	/if (!${Ini[${MacroData_Ini},File Paths,Loot Settings File].Length}) {
		/echo ERROR: Could not find designated file path for [Loot Settings File], please review [e3Data.ini] and restart.
		/endmacro
		/beep
	} else {
		/declare Loot_Ini string outer ${Ini[${MacroData_Ini},File Paths,Loot Settings File]}
	}

	/declare fullInventory_Alert bool outer FALSE
	/declare canLootCorpse bool outer FALSE
	/declare cantLootCorpseCount int outer FALSE
	/declare lootSetting string outer
	/declare c_itemValue string outer

	/if (!${Ini[${Loot_Ini}].Length}) /call Build_Alphabetized_Ini "${Loot_Ini}"
	/call iniToVarV "${genSettings_Ini},Loot,Loot Link Channel" linkChannel string outer
	/call iniToVarV "${genSettings_Ini},Loot,Corpse Seek Radius" seek_Radius int outer
	/call iniToVarV "${genSettings_Ini},Loot,Loot in Combat" combatLooting bool outer
	| Import character settings
	/if (${Bool[${Ini[${Character_Ini},Misc,Auto-Loot (On/Off)]}]}) /call iniToVarV "${Character_Ini},Misc,Auto-Loot (On/Off)" Auto_Loot bool outer
	/if (${Bool[${Ini[${Character_Ini},Misc,Loot On XP (On/Off)]}]}) /call iniToVarV "${Character_Ini},Misc,Loot On XP (On/Off)" Xp_Loot bool outer
/RETURN

|--------------------------------------------------------------------|
SUB loot_MacroSettings
	/call WriteToIni "${advSettings_Ini},Debug,Debug Loot (On/Off)" Off
	/call WriteToIni "${genSettings_Ini},Loot,Loot Link Channel" say
	/call WriteToIni "${genSettings_Ini},Loot,Corpse Seek Radius" 125
	/call WriteToIni "${genSettings_Ini},Loot,Loot in Combat" TRUE
/RETURN

|--------------------------------------------------------------------|
SUB loot_CharacterSettings
	/call WriteToIni "${Character_Ini},Misc,Auto-Loot (On/Off)" Off
	/call WriteToIni "${Character_Ini},Misc,Loot On XP (On/Off)" Off
/RETURN

|--------------------------------------------------------------------|
Sub loot_Aliases
	/squelch /alias /CheckLoot /bc Check Loot Setting Cursor
	/squelch /alias /SetLoot /bc Set Loot Setting Cursor
	/squelch /alias /SetLootKeep /bc Set Loot Setting Cursor /Keep
	/squelch /alias /SetLootSkip /bc Set Loot Setting Cursor /Skip
	/squelch /alias /SetLootDestroy /bc Set Loot Setting Cursor /Destroy
	/squelch /alias /LootItem /bc Loot Item ${Target.ID}
/return

|--------------------------------------------------------------------|
Sub loot_Background_Events
	/doevents lootRelay
	/doevents YourKill
	/doevents checkLootCursor
	/doevents updateLootCursor
	/doevents LootIt
	/doevents openClosestCorpse
	/doevents lootItem
/return