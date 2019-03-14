|------------------------------------------------------------------------|
|- e3_Macro_Heals.inc v2.0 ( originally designed to work with e3.mac )	-|
|- Written by Killians of PEQ											-|
|------------------------------------------------------------------------|
|- Contains heal related functions for e3.mac							-|
|------------------------------------------------------------------------|


| ${NetBots["Name"].PctHPs} sometimes returns the completely wrong info, 
| so replaced all use it with ${Spawn[ID ${NetBots["name"].ID}].PctHPs}

|------------------------------------------------------------|
|- Heals.inc's mainLoop hook.								-|	
|------------------------------------------------------------|					
|- Calls heal functions, as an extension of the main loop.	-|	
|------------------------------------------------------------|
SUB check_Heals
	/doevents wordHeal
	/if (${do_GroupHeals}) 						/call groupHeals
	/if (!${ActionTaken} && ${healTanks}) 		/call heal_tanks
	/if (!${ActionTaken} && ${healImportant})	/call heal_importantBots
	/if (!${ActionTaken} && ${healAll}) 		/call heal_allbots
	/if (!${ActionTaken} && ${hotTanks}) 		/call hot_tanks
	/if (!${ActionTaken} && ${hotImportant}) 	/call hot_importantBots
	/if (!${ActionTaken} && ${hotAll}) 			/call hot_allbots
	/if (!${ActionTaken} && ${healPets}) 		/call heal_Pets
	/if (!${ActionTaken} && ${hotPets}) 		/call hot_Pets
	/if (!${ActionTaken} && ${healOOC} && ${Me.CombatState.NotEqual[COMBAT]}) /call heal_OOCbots
/return

|--------------------------------------------------------------------------------------------------------------------|
|- Uses Items, Spells, Abilities, AAs, and Disciplines listed in the [LifeSupport] section of the Character_Ini.	-|
|--------------------------------------------------------------------------------------------------------------------|
SUB check_lifeSupport
	/declare s int local
	| only use life support when near mobs
	/if  (${SpawnCount[npc radius 200]} > 0) {
		/for s 1 to ${lifeSupport2D.Size[1]}
			/if (${Me.PctHPs} <= ${lifeSupport2D[${s},${iHealPct}]}) {
				/call check_Ready "lifeSupport2D" "${s}"
				/if (${c_Ready} && ${lifeSupport2D[${s},${iIfs}]}) {
					/if (${Bool[${Me.AbilityReady[${lifeSupport2D[${s},${iCastName}]}]}]}) {
						/if (${Me.Casting.ID} && ${Me.Class.ShortName.NotEqual[BRD]}) /call interrupt
						/doability "${lifeSupport2D[${s},${iCastName}]}"
					} else {
						/if (${check_Mana["lifeSupport2D",${s}]}) {
							/if (${Me.Casting.ID} && ${Me.Class.ShortName.NotEqual[BRD]}) /call interrupt
							/call e3_Cast ${Me.ID} "lifeSupport2D" ${s}
						}
					}
				}
			}
		/next s
	}
/RETURN

|-------------------------------------------------------|
|- Find the first tank below healpct to heal           -|
|- ?? Shouldn't we find the tank with the lowest hp ?? -|
|-------------------------------------------------------|
SUB heal_tanks
	/declare selectedBot int local 0
	/declare selectedPctHPs int local 100
	/declare selectedSpellIndx int local
	/declare currentBot int local
	/declare currentPctHPs int local
	/declare t int local
	/declare s int local
	/for t 1 to ${tanks.Size}
		| Why check every heal spell on bots that aren't even in the zone?
		/if (${Bool[${NetBots[${tanks[${t}]}].InZone}]}) {
			/varset currentBot ${NetBots[${tanks[${t}]}].ID}
			/varset currentPctHPs ${Spawn[ID ${currentBot}].PctHPs}
			/for s 1 to ${tankHeals2D.Size[1]}
				|/if ($${NetBots[${tanks[${t}]}].PctHPs} <= ${tankHeals2D[${s},${iHealPct}]}) {
				/if (${currentPctHPs} <= ${tankHeals2D[${s},${iHealPct}]}) {
					/if (${check_Distance[${currentBot},${tankHeals2D[${s},${iMyRange}]}]}) {
						/call check_Ready "tankHeals2D" ${s}
						/if (${c_Ready} && ${tankHeals2D[${s},${iIfs}]}) {
							/if (${check_Mana["tankHeals2D",${s}]}) {
								/if (${currentPctHPs} < ${selectedPctHPs}) {
									/varset selectedBot ${currentBot}
									/varset selectedSpellIndx ${s}
									/varset selectedPctHPs ${currentPctHPs}
								}
							}
						}
					}
				}
			/next s
		}
	/next t

	/if (${selectedBot} != 0) {
		/call e3_Cast ${selectedBot} "tankHeals2D" ${selectedSpellIndx}
	}	
	
/RETURN

|------------------------------------------------|
|- Find the lowest hp importantBot to heal      -|
|------------------------------------------------|
SUB heal_importantBots
	/declare selectedSpellIndx int local
	/declare selectedPctHPs int local 100
	/declare selectedBot int local
	/declare currentBot int local
	/declare currentPctHPs int local
	/declare t int local
	/declare s int local
	/for t 1 to ${importantBots.Size}
		/if (${Bool[${NetBots[${importantBots[${t}]}].InZone}]}) {
			/varset currentBot ${NetBots[${importantBots[${t}]}].ID}
			/varset currentPctHPs ${Spawn[ID ${currentBot}].PctHPs}
			/for s 1 to ${importantHeals2D.Size[1]}
				/if (${currentPctHPs} <= ${importantHeals2D[${s},${iHealPct}]}) {
					/if (${check_Distance[${currentBot},${importantHeals2D[${s},${iMyRange}]}]}) {
						/call check_Ready "importantHeals2D" ${s}
						/if (${c_Ready} && ${importantHeals2D[${s},${iIfs}]}) {
							/call check_Distance
							/if (${check_Mana["importantHeals2D",${s}]}) {
								/if (${currentPctHPs} < ${selectedPctHPs}) {
									/varset selectedBot ${currentBot}
									/varset selectedSpellIndx ${s}
									/varset selectedPctHPs ${currentPctHPs}
								}
							}
						}
					}
				}
			/next s
		}
	/next t

	/if (${selectedBot} != 0) {
		/call e3_Cast ${selectedBot} "importantHeals2D" ${selectedSpellIndx}
	}
/RETURN

|------------------------------------------------|
|- Find the lowest hp Netbot to heal			-|
|------------------------------------------------|
SUB heal_Allbots
	/declare selectedSpellIndx int local
	/declare selectedPctHPs int local 100
	/declare selectedBot int local
	/declare currentBot int local 
	/declare currentPctHPs int local
	/declare currClient string local NULL
	/declare t int local 1
	/declare s int local 1
	
	|Randomly Iterate forward or backwards to reduce overheal
	/if (${Math.Rand[2]}==0) {
		/for t 1 to ${NetBots.Counts}
	} else {
		/for t ${NetBots.Counts} downto 1
	}
	
		/varset currClient ${NetBots.Client[${t}]}
		/if (${Bool[${NetBots[${currClient}].InZone}]}) {
			/varset currentBot ${NetBots[${currClient}].ID}
			/varset currentPctHPs ${Spawn[ID ${currentBot}].PctHPs}
			/for s 1 to ${allHeals2D.Size[1]}
				/if (${currentPctHPs} <= ${allHeals2D[${s},${iHealPct}]}) {
					/if (${check_Distance[${currentBot},${allHeals2D[${s},${iMyRange}]}]}) {
						/call check_Ready "allHeals2D" ${s}
						/if (${c_Ready} && ${allHeals2D[${s},${iIfs}]}) {
							/if (${check_Mana["allHeals2D",${s}]}) {
								/if (${currentPctHPs} < ${selectedPctHPs}) {
									/varset selectedBot ${currentBot}
									/varset selectedSpellIndx ${s}
									/varset selectedPctHPs ${currentPctHPs}
								}
							}
						}
					}
				}
			/next s
		}
	/next t

	/if (${selectedBot} != 0) {	
		/call e3_Cast ${selectedBot} "allHeals2D" ${selectedSpellIndx}	
|		/call set_RecentHealTimer "${allHeals2D[${s},${iCastType}]}"
	}	
/RETURN

|------------------------------------------------|
|- Find the lowest HP OOC Netbot to heal		-|
|------------------------------------------------|
SUB heal_OOCbots
	/declare selectedSpellIndx int local
	/declare selectedPctHPs int local 100
	/declare selectedBot int local
	/declare currentBot int local
	/declare currentPctHPs int local
	/declare currClient string local NULL
	/declare t int local 1
	/declare s int local 1
	
	|Randomly Iterate forward or backwards to reduce overheal
	/if (${Math.Rand[2]}==0) {
		/for t 1 to ${NetBots.Counts}
	} else {
		/for t ${NetBots.Counts} downto 1
	}
	
		/varset currClient ${NetBots.Client[${t}]}
		/if (${Bool[${NetBots[${currClient}].InZone}]} && ${NetBots[${currClient}].CombatState} != 0) {
			/for s 1 to ${oocHeals2D.Size[1]}
				/varset currentBot ${NetBots[${currClient}].ID}
				/varset currentPctHPs ${Spawn[ID ${currentBot}].PctHPs}
				/if (${currentPctHPs} <= ${oocHeals2D[${s},${iHealPct}]}) {
					/if (${check_Distance[${currentBot},${oocHeals2D[${s},${iMyRange}]}]}) {
						/call check_Ready "oocHeals2D" ${s}
						/if (${c_Ready} && ${oocHeals2D[${s},${iIfs}]}) {
							/if (${check_Mana["oocHeals2D",${s}]}) {
								/if (${currentPctHPs} < ${selectedPctHPs}) {
									/varset selectedBot ${currentBot}
									/varset selectedSpellIndx ${s}
									/varset selectedPctHPs ${currentPctHPs}
								}
							}
						}
					}
				}
			/next s
		}
	/next t

	/if (${selectedBot} != 0) {	
		/call e3_Cast ${selectedBot} "oocHeals2D" ${selectedSpellIndx}	
|		/call set_RecentHealTimer "${oocHeals2D[${s},${iCastType}]}"
	}	
/RETURN

|------------------------------------------------|
|- Find the lowest hp Pet to heal				-|
|------------------------------------------------|
SUB heal_pets
	/declare currentBot string local NULL
	/declare currentSpellIndx int local
	/declare t int local
	/declare s int local
	/for t 1 to ${petOwners.Size}
		/if (${Bool[${NetBots[${petOwners[${t}]}].InZone}]} && ${Spawn[pc =${petOwners[${t}]}].Pet.ID}) {
			/for s 1 to ${petHeals2D.Size[1]}
				/if (${NetBots[${petOwners[${t}]}].PetHP} <= ${petHeals2D[${s},${iHealPct}]}) {
					/call check_Ready "petHeals2D" ${s}
					/if (${c_Ready} && ${petHeals2D[${s},${iIfs}]}) {
						/if (${Spawn[pc =${petOwners[${t}]}].Pet.Distance} < ${petHeals2D[${s},${iMyRange}]}) {
							/if (${check_Mana["petHeals2D",${s}]}) {
								/if (!${Bool[${currentBot}]} || ${NetBots[${petOwners[${t}]}].PetHP} < ${NetBots[${currentBot}].PetHP}) {
									/varset currentBot ${petOwners[${t}]}
									/varset currentSpellIndx ${s}
								}
							}
						}
					}
				}
			/next s
		}
	/next t
	/if (${Bool[${currentBot}]}) {
		/call e3_Cast ${NetBots[${currentBot}].PetID} "petHeals2D" ${currentSpellIndx}
	}
/RETURN

|--------------------------------------------------|
|- Find the tank first tank below healpct to hot  -|
|--------------------------------------------------|
SUB hot_tanks
	/declare currentBot string local NULL
	/declare currentSpellIndx int local	
	/declare t int local
	/declare s int local		
	/for t 1 to ${tanks.Size}
		/for s 1 to ${hotSpells2D.Size[1]}
			/if (${Bool[${NetBots[${tanks[${t}]}].InZone}]} && ${NetBots[${tanks[${t}]}].PctHPs} <= ${hotSpells2D[${s},${iHealPct}]}) {
				/call check_HotTankStack "${tanks[${t}]}" ${hotSpells2D[${s},${iSpellID}]}
				/if (${c_SubToRun}) {
					/call check_Ready "hotSpells2D" ${s}
					/if (${c_Ready} && ${hotSpells2D[${s},${iIfs}]}) {
						/if (${check_Distance[${NetBots[${tanks[${t}]}].ID},${hotSpells2D[${s},${iMyRange}]}]}) {
							/if (${check_Mana["hotSpells2D",${s}]}) {
								/varset currentBot ${tanks[${t}]}
								/varset currentSpellIndx ${s}
							}
						}
					}
				}
			}
		/if (!${Bool[${currentBot}]}) /next s
	/if (!${Bool[${currentBot}]}) /next t
	
	/if (${Bool[${currentBot}]}) {
		/varset hotSpells2D[${currentSpellIndx},${iSubToRun}] check_HotTankStack
		/call e3_Cast ${NetBots[${currentBot}].ID} "hotSpells2D" ${currentSpellIndx}
|		/call set_RecentHealTimer "${hotSpells2D[${currentSpellIndx},${iCastType}]}"
	}	
/RETURN

|------------------------------------------------|
|- Find the lowest hp importantBot to hot		-|
|------------------------------------------------|
SUB hot_importantBots
	/declare currentBot string local NULL
	/declare currentSpellIndx int local
	/declare t int local
	/declare s int local
	/for t 1 to ${importantBots.Size}
	/for s 1 to ${hotSpells2D.Size[1]}
	/if (${Bool[${NetBots[${importantBots[${t}]}].InZone}]} && ${NetBots[${importantBots[${t}]}].PctHPs} <= ${hotSpells2D[${s},${iHealPct}]}) {
		/call check_HotImportantBotStack "${importantBots[${t}]}" ${hotSpells2D[${s},${iSpellID}]}
		/if (${c_SubToRun}) {
			/call check_Ready "hotSpells2D" ${s}
			/if (${c_Ready} && ${hotSpells2D[${s},${iIfs}]}) {
				/if (${check_Distance[${NetBots[${importantBots[${t}]}].ID},${hotSpells2D[${s},${iMyRange}]}]}) {
					/if (${check_Mana["hotSpells2D",${s}]}) {
						/if (!${Bool[${currentBot}]} || ${NetBots[${importantBots[${t}]}].PctHPs} < ${NetBots[${currentBot}].PctHPs}) {
							/varset currentBot ${importantBots[${t}]}
							/varset currentSpellIndx ${s}
						}
					}
				}
			}
		}
	}
	/next s
	/next t

	/if (${Bool[${currentBot}]}) {
		/varset hotSpells2D[${currentSpellIndx},${iSubToRun}] check_HotImportantBotStack
		/call e3_Cast ${NetBots[${currentBot}].ID} "hotSpells2D" ${currentSpellIndx}
	}
/RETURN

|------------------------------------------------|
|- Find the lowest hp Netbot to hot				-|
|------------------------------------------------|
SUB hot_Allbots
	/declare currentBot string local NULL
	/declare currClient string local NULL
	/declare currentSpellIndx int local	
	/declare t int local 1
	/declare s int local 1
	
	|Randomly Iterate forward or backwards to reduce overheal
	/if (${Math.Rand[2]}==0) {
		/for t 1 to ${NetBots.Counts}
	} else {
		/for t ${NetBots.Counts} downto 1
	}
	/varset currClient ${NetBots.Client[${t}]}
		/for s 1 to ${hotSpells2D.Size[1]}
			/if (${Bool[${NetBots[${currClient}].InZone}]} && ${NetBots[${currClient}].PctHPs} <= ${hotSpells2D[${s},${iHealPct}]}) {
				/call check_HotAllStack "${currClient}" ${hotSpells2D[${s},${iSpellID}]}
				/if (${c_SubToRun}) {
					/call check_Ready "hotSpells2D" ${s}
					/if (${c_Ready} && ${hotSpells2D[${s},${iIfs}]}) {
						/if (${check_Distance[${NetBots[${currClient}].ID},${hotSpells2D[${s},${iMyRange}]}]}) {
							/if (${check_Mana["hotSpells2D",${s}]}) {
								/if (!${Bool[${currentBot}]} || ${NetBots[${currClient}].PctHPs} < ${NetBots[${currentBot}].PctHPs}) {
									/varset currentBot ${currClient}
									/varset currentSpellIndx ${s}
								}
							}
						}
					}
				}
			}
		/next s
	/next t
	
	/if (${Bool[${currentBot}]}) {
		/varset hotSpells2D[${currentSpellIndx},${iSubToRun}] check_HotAllStack
		/call e3_Cast ${NetBots[${currentBot}].ID} "hotSpells2D" ${currentSpellIndx}	
|		/call set_RecentHealTimer "${hotSpells2D[${s},${iCastType}]}"
	}	
/RETURN

|------------------------------------------------|
|- Find the lowest hp Pet to hot				-|
|------------------------------------------------|
SUB hot_pets
	/declare currentBot string local NULL
	/declare currentSpellIndx int local
	/declare t int local
	/declare s int local
	/for t 1 to ${petOwners.Size}
	|/echo ${petOwners[${t}]} ${Spawn[pc =${petOwners[${t}]}].Pet.Name} hp ${NetBots[${petOwners[${t}]}].PetHP}
	/if (${Bool[${NetBots[${petOwners[${t}]}].InZone}]} && ${Spawn[pc =${petOwners[${t}]}].Pet.ID}) {
		/for s 1 to ${hotSpells2D.Size[1]}
		/if (${NetBots[${petOwners[${t}]}].PetHP} <= ${hotSpells2D[${s},${iHealPct}]}) {
			/call check_HotPetStack "${petOwners[${t}]}" ${hotSpells2D[${s},${iSpellID}]}
			/if (${c_SubToRun}) {
				/call check_Ready "hotSpells2D" ${s}
				/if (${c_Ready} && ${hotSpells2D[${s},${iIfs}]}) {
					/if (${Spawn[pc =${petOwners[${t}]}].Pet.Distance} < ${hotSpells2D[${s},${iMyRange}]}) {
						/if (${check_Mana["hotSpells2D",${s}]}) {
							/if (!${Bool[${currentBot}]} || ${NetBots[${petOwners[${t}]}].PetHP} < ${NetBots[${currentBot}].PetHP}) {
								/varset currentBot ${petOwners[${t}]}
								/varset currentSpellIndx ${s}
							}
						}
					}
				}
			}
		}
		/next s
	}
	/next t
	|/echo ${currentBot} ${Spawn[pc =${currentBot}].Pet.ID} ${Spawn[pc =${currentBot}].Pet.Name}
	/if (${Bool[${currentBot}]}) {
		/varset hotSpells2D[${currentSpellIndx},${iSubToRun}] check_HotPetStack
		/call e3_Cast ${NetBots[${currentBot}].PetID} "hotSpells2D" ${currentSpellIndx}
	}
/RETURN

|------------------------------------------------|
|- Check Tank hot stacking					    -|
|------------------------------------------------|
SUB check_HotTankStack(targetName, spellID)
	/varset c_SubToRun TRUE
	/declare i int local
	/if (!${Defined[targetName]}) {
		/declare targShortBuff string local ${NetBots[${Target.CleanName}].ShortBuff.Replace[ ,,]}
	} else {
		/declare targShortBuff string local ${NetBots[${targetName}].ShortBuff.Replace[ ,,]}
	}
	/if (${targShortBuff.Find[${spellID},]}) {
		/varset c_SubToRun FALSE
	} else {
		/for i 1 to ${hotTankNoStack.Size}
			/if (${Select[${hotTankNoStack[${i}]},${targShortBuff}]}) /varset c_SubToRun FALSE
		/next i
	}
/RETURN

|------------------------------------------------|
|- Check Important Bot hot stacking				-|
|------------------------------------------------|
SUB check_HotImportantBotStack(targetName, spellID)
	/varset c_SubToRun TRUE
	/declare i int local
	/if (!${Defined[targetName]}) {
		/declare targShortBuff string local ${NetBots[${Target.CleanName}].ShortBuff.Replace[ ,,]}
	} else {
		/declare targShortBuff string local ${NetBots[${targetName}].ShortBuff.Replace[ ,,]}
	}
	/if (${Bool[${spellID}]} && ${targShortBuff.Find[${spellID},]}) {
		/varset c_SubToRun FALSE
	} else {
		/for i 1 to ${hotImportantNoStack.Size}
			/if (${Select[${hotImportantNoStack[${i}]},${targShortBuff}]}) /varset c_SubToRun FALSE
		/next i
	}
/RETURN

|------------------------------------------------|
|- Check Netbot Bot hot stacking				-|
|------------------------------------------------|
SUB check_HotAllStack(targetName, spellID)
	/varset c_SubToRun TRUE
	/declare i int local
	/if (!${Defined[targetName]}) {
		/declare targShortBuff string local ${NetBots[${Target.CleanName}].ShortBuff.Replace[ ,,]}
	} else {
		/declare targShortBuff string local ${NetBots[${targetName}].ShortBuff.Replace[ ,,]}
	}
	/if (${Bool[${spellID}]} && ${targShortBuff.Find[${spellID},]}) {
		/varset c_SubToRun FALSE
	} else {
		/for i 1 to ${hotAllNoStack.Size}
			/if (${Select[${hotAllNoStack[${i}]},${targShortBuff}]}) /varset c_SubToRun FALSE
		/next i
	}
/RETURN

|------------------------------------------------|
|- Check Pet hot stacking				-|
|------------------------------------------------|
SUB check_HotPetStack(targetName, spellID)
	/varset c_SubToRun TRUE
	/declare i int local
	/if (!${Defined[targetName]}) {
		/declare targShortBuff string local ${NetBots[${Target.CleanName}].PetBuff.Replace[ ,,]}
	} else {
		/declare targShortBuff string local ${NetBots[${targetName}].PetBuff.Replace[ ,,]}
	}
	/if (${Bool[${spellID}]} && ${targShortBuff.Find[${spellID},]}) {
		/varset c_SubToRun FALSE
	} else {
		/for i 1 to ${hotAllNoStack.Size}
			/if (${Select[${hotAllNoStack[${i}]},${targShortBuff}]}) /varset c_SubToRun FALSE
		/next i
	}
/RETURN


|----------------------------------------|
|- Checks while casting tank heals		-|
|----------------------------------------|
SUB check_HealTankCasting
	/varset c_SubToRun FALSE
	/if (${castEndTime} > ${interruptTankCastSec} || ${Spawn[id ${Target.ID}].PctHPs} < ${interruptHealPct}) /varset c_SubToRun TRUE
	|/echo cth ${Target.CleanName} nb ${NetBots[${Target.CleanName}].PctHPs} s ${Spawn[${Target.CleanName}].PctHPs} cet ${castEndTime} ${passCheck} ${interruptTankCastSec}
/RETURN
|----------------------------------------|
|- Checks while casting important heals	-|
|----------------------------------------|
SUB check_HealImportantCasting
	/varset c_SubToRun FALSE
	/if (${castEndTime} > ${interruptImportantCastSec} || ${Spawn[id ${Target.ID}].PctHPs} < ${interruptHealPct}) /varset c_SubToRun TRUE
/RETURN
|----------------------------------------|
|- Checks while casting all heals		-|
|----------------------------------------|
SUB check_HealAllCasting
	/varset c_SubToRun FALSE
	/if (${castEndTime} > ${interruptAllCastSec} || ${Spawn[id ${Target.ID}].PctHPs} < ${interruptHealPct}) /varset c_SubToRun TRUE
/RETURN
|----------------------------------------|
|- Checks while casting all heals		-|
|----------------------------------------|
SUB check_HealPetCasting
	/varset c_SubToRun FALSE
	/if (${castEndTime} > ${interruptAllCastSec} || ${Spawn[id ${Target.ID}].PctHPs} < ${interruptHealPct}) /varset c_SubToRun TRUE
/RETURN

|-------------------------------------------|
|-Set Recent Heal Timer Based on CastReturn-|
|-------------------------------------------|
|SUB set_RecentHealTimer(CastType)
|	/if (${Cast.Result.Equal[CAST_SUCCESS]} && ${CastType.Equal[Spell]}) {
|		/varset recentHeal_Timer 1s
|	} else {
|		/varset recentHeal_Timer .25s
|	}
|/RETURN

|----------------------------------------|
|- Checks while casting group heals		-|
|----------------------------------------|
SUB check_HealGroupCasting(ArrayName, int ArrayIndex)
	/varset c_SubToRun FALSE
	/declare avgGrpHealth	int local ${Me.PctHPs}
	/declare numLowHp		int local 0
	/declare targetCount	int local 1
	/declare g				int local
	/if (${Me.PctHPs} <= ${${ArrayName}[${ArrayIndex},${iHealPct}]}) /varset numLowHp 1
	| Check range of group members, count number below pcthp and add their healths together.
	/for g 1 to ${Group}
		/if (${check_Distance[${Group.Member[${g}].ID},${${ArrayName}[${ArrayIndex},${iMyRange}]}]}) {
			/varcalc targetCount ${targetCount} + 1
			/varcalc avgGrpHealth ${avgGrpHealth} + ${NetBots[${Group.Member[${g}]}].PctHPs}
			/if (${Bool[${NetBots[${Group.Member[${g}]}].ID}]} && ${NetBots[${Group.Member[${g}]}].PctHPs}  <= ${${ArrayName}[${ArrayIndex},${iHealPct}]}) /varcalc numLowHp ${numLowHp} + 1
		}
	/next g
	| must have at least 3 targets and avg group health below HealPct					
	/varcalc avgGrpHealth ${avgGrpHealth} / ${targetCount}
	/if (${numLowHp} > 2 && ${avgGrpHealth} <= ${${ArrayName}[${ArrayIndex},${iHealPct}]}) /varset c_SubToRun TRUE
/RETURN

|-----------------------------------------------|
| Cast group heals when group has
|- > 2 members below HealPct 
|- Avg group health <= HealPct 
|-----------------------------------------------|
SUB groupHeals
	/if (${Group}) {
		/declare healTarget int local
		/declare s int local
		
		/for s 1 to ${groupHeals2D.Size[1]}
			/call check_HealGroupCasting "groupHeals2D" ${s}
			/if (${c_SubToRun}) {
				/if (${check_Mana["groupHeals2D",${s}]}) {
					/call check_Ready "groupHeals2D" ${s}
					/if (${c_Ready} && ${groupHeals2D[${s},${iIfs}]}) {
						/call e3_Cast ${Me.ID} "groupHeals2D" ${s}
					}
				}
			}
		/next s
	}
/RETURN

|--------------------------------------------------------------------|
|- setup_Heals														-|
|--------------------------------------------------------------------|
|- Imports and validates ini settings from Character Ini,[Heals].	-|
|--------------------------------------------------------------------|
SUB heal_Setup
/call iniToVarV "${advSettings_Ini},Debug,Debug Heals (On/Off)" Debug_Heals bool outer
	| CONFIGURABLE
	|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
	|~~~~~~~~~~~~~~~~~~~ Hot Stacking Definition ~~~~~~~~~~~~~~~~~~~|
	|~~~~~~~~~~~ Included Spell ID will NOT be overriden ~~~~~~~~~~~|
	| 5259 Pious Elixir			- 1170/tick
	| 8493 Elixir of Divinity	- 900/tick
	| 5416 Spiritual Serenity	- 820/tick
	| 8503 Ghost of Renewal		- 630/tick	
	/declare hotTankNoStackStr		string outer 5259,8493
	/declare hotImportantNoStackStr	string outer 5259,8493,5416,8503
	/declare hotAllNoStackStr		string outer 5259,8493,5416,8503
	|~~~~~~~~~~~~~~~~~~~ PCTHps Interrupt	~~~~~~~~~~~~~~~~~~~~~~~~|
	|~~~~~~~~~~~~ Interrupt heal if target >= this PCTHps ~~~~~~~~~~|
	/declare interruptHealPct			int outer 95
	|~~~~ Only interrupt if cast time left <=  (1/10th seconds) ~~~~|
	/declare interruptTankCastSec		int outer 3
	/declare interruptImportantCastSec	int outer 18
	/declare interruptAllCastSec		float outer 25

	/declare do_GroupHeals bool outer FALSE
	/declare healTanks bool outer FALSE
	/declare healImportant bool outer FALSE
	/declare healAll bool outer FALSE
	/declare healPets bool outer FALSE
	/declare healOOC bool outer FALSE
	/declare hotTanks bool outer FALSE
	/declare hotImportant bool outer FALSE
	/declare hotAll bool outer FALSE
	/declare hotPets bool outer FALSE

	/call ListToArray "hotTankNoStack"		"${hotTankNoStackStr}"		","
	/call ListToArray "hotImportantNoStack" "${hotImportantNoStackStr}"	","
	/call ListToArray "hotAllNoStack"		"${hotAllNoStackStr}" 		","

	/if (${Bool[${Ini[${Character_Ini},Heals]}]}) {
		/if (${Ini[${Character_Ini},Heals,Who to Heal].Length})		/call iniToVarV "${Character_Ini},Heals,Who to Heal"	 WhoToHeal string outer
		/if (${Ini[${Character_Ini},Heals,Who to HoT].Length})		/call iniToVarV "${Character_Ini},Heals,Who to HoT"		 WhoToHoT string outer
		/call IniToArrayV "${Character_Ini},Heals,Tank#"           tanks
		/call IniToArrayV "${Character_Ini},Heals,Important Bot#"  importantBots
		/call IniToArrayV "${Character_Ini},Heals,Pet Owner#"      petOwners
	}

	/call heal_SpellArrays
/RETURN
|--------------------------------------------------------------------------------------------------------------------------------
|--------------------------------------------------------------------------------------------------------------------------------
SUB heal_SpellArrays
| Import settings from Character_Ini
	/call IniToArrayV "${Character_Ini},Life Support,Life Support#" lifeSupport
	/if (${Bool[${Ini[${Character_Ini},Heals]}]}) {
		/call IniToArrayV "${Character_Ini},Heals,Tank Heal#"				tankHeals
		/call IniToArrayV "${Character_Ini},Heals,Important Heal#"			importantHeals
		/call IniToArrayV "${Character_Ini},Heals,All Heal#"				allHeals
		/call IniToArrayV "${Character_Ini},Heals,Pet Heal#"				petHeals
		/call IniToArrayV "${Character_Ini},Heals,Heal Over Time Spell#"	hotSpells
		/call IniToArrayV "${Character_Ini},Heals,Group Heal#"				groupHeals
		/call IniToArrayV "${Character_Ini},Heals,OOC Heal#"				oocHeals
	}
|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
| lets healers break invis to cast heal
	/declare castGroupHealWhileInvis		int local 1
	/declare castTankHealWhileInvis			int local 1
	/declare castImportantHealWhileInvis	int local 1
	/declare castAllHealWhileInvis			int local 1

	/if (${Defined[WhoToHeal]}) {
		/if (${WhoToHeal.Find[Tanks]} && ${Defined[tanks]} && ${Defined[tankHeals]})						/varset healTanks		TRUE
		/if (${WhoToHeal.Find[ImportantBots]} && ${Defined[importantBots]} && ${Defined[importantHeals]})	/varset healImportant	TRUE
		/if (${WhoToHeal.Find[All]} && ${Defined[allHeals]})												/varset healAll			TRUE
		/if (${WhoToHeal.Find[Pets]} && ${Defined[petOwners]} && ${Defined[petHeals]})						/varset healPets		TRUE
		/if (${WhoToHeal.Find[OOC]} && ${Defined[oocHeals]})												/varset healOOC			TRUE
	}
	/if (${Defined[WhoToHoT]}) {
		/if (${WhoToHoT.Find[Tanks]} && ${Defined[tanks]} && ${Defined[hotSpells]})							/varset hotTanks		TRUE
		/if (${WhoToHoT.Find[ImportantBots]} && ${Defined[importantBots]} && ${Defined[hotSpells]})			/varset hotImportant	TRUE
		/if (${WhoToHoT.Find[All]} && ${Defined[hotSpells]})												/varset hotAll			TRUE
		/if (${WhoToHoT.Find[Pets]} && ${Defined[petOwners]} && ${Defined[hotSpells]})						/varset hotPets			TRUE
	}
	/if (${Defined[groupHeals]})																			/varset do_GroupHeals	TRUE

	/if (${do_GroupHeals})			/call BuildSpellArray "groupHeals"			"groupHeals2D"
	/if (${healTanks})				/call BuildSpellArray "tankHeals" 			"tankHeals2D"
	/if (${healImportant})			/call BuildSpellArray "importantHeals"		"importantHeals2D"
	/if (${healAll})				/call BuildSpellArray "allHeals" 			"allHeals2D"
	/if (${healPets})				/call BuildSpellArray "petHeals" 			"petHeals2D"
	/if (${Defined[hotSpells]})		/call BuildSpellArray "hotSpells" 			"hotSpells2D"
	/if (${Defined[lifeSupport]})	/call BuildSpellArray "lifeSupport" 		"lifeSupport2D"
	/if (${healOOC})				/call BuildSpellArray "oocHeals" 			"oocHeals2D"

|Set SubToRun for Heals, Hots are handled in respective function
	/declare i int local
	/if (${do_GroupHeals}) {
		/for i 1 to ${groupHeals2D.Size[1]}
			/varset groupHeals2D[${i},${iSubToRun}] check_HealGroupCasting
			/varset groupHeals2D[${i},${iCastInvis}] ${castGroupHealWhileInvis}
		/next i
	}
	/if (${healTanks}) {
		/for i 1 to ${tankHeals2D.Size[1]}
			/varset tankHeals2D[${i},${iSubToRun}] check_HealTankCasting
			/varset tankHeals2D[${i},${iCastInvis}] ${castTankHealWhileInvis}
		/next i
	}
	/if (${healImportant}) {
		/for i 1 to ${importantHeals2D.Size[1]}
			/varset importantHeals2D[${i},${iSubToRun}] check_HealImportantCasting
			/varset importantHeals2D[${i},${iCastInvis}] ${castImportantHealWhileInvis}
		/next i
	}
	/if (${healAll}) {
		/for i 1 to ${allHeals2D.Size[1]}
			/varset allHeals2D[${i},${iSubToRun}] check_HealAllCasting
			/varset allHeals2D[${i},${iCastInvis}] ${castAllHealWhileInvis}
		/next i
	}
	/if (${healPets}) {
		/for i 1 to ${petHeals2D.Size[1]}
			/varset petHeals2D[${i},${iSubToRun}] check_HealPetCasting
		/next i
	}
/RETURN

SUB heal_MacroSettings
	/call WriteToIni "${advSettings_Ini},Debug,Debug Heals (On/Off)" Off
/RETURN

SUB heal_CharacterSettings
	| LifeSupport entries.
	/call WriteToIni "${Character_Ini},Life Support,Life Support"
	| If I'm a healer, add e3_Macro_Heals.inc entries.
	/if (${Select[${Me.Class.ShortName},CLR,DRU,SHM,BST,PAL,RNG]}) {
		/call WriteToIni "${Character_Ini},Heals,Tank"
		/call WriteToIni "${Character_Ini},Heals,Important Bot"
		/call WriteToIni "${Character_Ini},Heals,Pet Owner"
		/call WriteToIni "${Character_Ini},Heals,Tank Heal"
		/call WriteToIni "${Character_Ini},Heals,Important Heal"
		/call WriteToIni "${Character_Ini},Heals,All Heal"
		/call WriteToIni "${Character_Ini},Heals,Pet Heal"
		/call WriteToIni "${Character_Ini},Heals,OOC Heal"
		/if (${Select[${Me.Class.ShortName},CLR,DRU,PAL,SHM]}) /call WriteToIni "${Character_Ini},Heals,Heal Over Time Spell"
		/if (${Select[${Me.Class.ShortName},CLR,DRU,PAL]})	/call WriteToIni "${Character_Ini},Heals,Group Heal"
		|/if (${Select[${Me.Class.ShortName},CLR,SHM]}) /call WriteToIni "${Character_Ini},Heals,;Group Hot" "Not Implemented"
		/call WriteToIni "${Character_Ini},Heals,Who to Heal" "Tanks/ImportantBots/All/Pets/OOC"
		/if (${Select[${Me.Class.ShortName},CLR,DRU,PAL,SHM]}) /call WriteToIni "${Character_Ini},Heals,Who to HoT"
	}
/RETURN

Sub heal_Background_Events
/return

Sub heal_Aliases
/return
