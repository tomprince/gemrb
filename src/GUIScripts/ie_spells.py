# !!! Keep these synchronized with Spellbook.h !!!

# LearnSpell flags
LS_ADDXP = 1   #give xp for learning it
LS_LEARN = 2   #give message when learned it
LS_STATS = 4   #check stats (alignment, etc)
LS_MEMO  = 8   #memorize it instantly (add innate)
LS_NOXP  = 16  #don't give xp; has precedence over LS_ADDXP

# LearnSpell return values
LSR_OK      = 0
LSR_KNOWN   = 1  #already knows
LSR_INVALID = 2  #invalid resref
LSR_FAILED  = 3  #failed stat roll
LSR_STAT    = 4  #insufficient stat (can't learn the spell due to low stat)
LSR_LEVEL   = 5  #insufficient level (low mage, etc level)
LSR_FULL    = 6  #can't learn more spells of this level (due to level)
