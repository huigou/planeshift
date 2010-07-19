# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 4.0.18-max-nt
#
# Table structure for table 'math_scripts'
#

DROP TABLE IF EXISTS `math_scripts`;
CREATE TABLE math_scripts (
  name varchar(40) NOT NULL DEFAULT '' ,
  math_script text NOT NULL,
  PRIMARY KEY (name)
);


#
# Dumping data for table 'math_scripts'
#

INSERT INTO math_scripts VALUES( "Calculate Damage",
"
	AttackRoll = rnd(1);
        DefenseRoll = rnd(1);

        WeaponSkill = Attacker:getAverageSkillValue(AttackWeapon:Skill1,AttackWeapon:Skill2,AttackWeapon:Skill3);
        TargetWeaponSkill = Target:getAverageSkillValue(TargetWeapon:Skill1,TargetWeapon:Skill2,TargetWeapon:Skill3);

        PI = 3.14159265358979323846;
        Dist = DiffX*DiffX + DiffY*DiffY + DiffZ*DiffZ;
        NDiffX = -DiffX;
        NDiffZ = -DiffZ;

        BadRange = pow(AttackWeapon:Range * 1.1 + 1,2) - Dist;
        exit = if(0>BadRange,1,0);

        Angle = atan2(NDiffX,NDiffZ);
        AngleAtt = Angle;
        Angle = Attacker:loc_yrot - Angle;
        Angle = Angle % (2*PI);
        Angle = if(Angle > PI, Angle-2*PI, Angle);
        BadAngle = PI * if(Dist>1.5, 0.3, 0.4) - abs(Angle);
        exit = if(0>BadAngle,1,0);

        Missed = min(AttackRoll-.25,0.1);
        exit = if(0>Missed,1,0);
        Dodged = min(AttackRoll-.5,.01);
        exit = if(0>Dodged,1,0);
        Blocked = AttackRoll - DefenseRoll;
        exit = if(0>Blocked,1,0);

        RequiredInputVars = Target:AttackerTargeted+Attacker:getSkillValue(AttackWeapon:Skill1)+AttackLocationItem:Hardness;

        AttackerStance = Attacker:CombatStance;
        TargetStance = Target:CombatStance;

        AttackValue = WeaponSkill;
        TargetAttackValue = TargetWeaponSkill;
        DV = Attacker:Agility;
        TargetDV = 0;

        AVStance = if(AttackerStance=1, (AttackValue*2)+(DV*0.8),
               if(AttackerStance=2, (AttackValue*1.5)+(DV*0.5),
               if(AttackerStance=3, AttackValue,
               if(AttackerStance=4, (AttackValue*0.3),
               0))));

        TargetAVStance = if(TargetStance=1, (TargetAttackValue*2)+(TargetDV*0.8),
               if(TargetStance=2, (TargetAttackValue*1.5)+(TargetDV*0.5),
               if(TargetStance=3, TargetAttackValue,
               if(TargetStance=4, (TargetAttackValue*0.3),
               0))));

        FinalDamage = 10*(AVStance-TargetDV);
");

INSERT INTO math_scripts VALUES( "Calculate Decay",
"
    WeaponDecay = if(Blocked, 1.25, 1 - ArmorVsWeapon);
    BlockingDecay = if(Blocked, 0.75, 0);
    ArmorDecay = if(Blocked, 0, ArmorVsWeapon);
");

INSERT INTO math_scripts VALUES( "Lockpicking Time", "Time = ((LockQuality / 10) * 3)*1000;");

INSERT INTO math_scripts VALUES( "Calculate Fall Damage",
"
	exit = (rnd(1) > 0.5);
        Damage = if(FallHeight>1,pow(FallHeight, 1.8) * 0.8, 0);
");

INSERT INTO math_scripts VALUES( "CalculateManaCost", "ManaCost = Realm*4*(1+(KFactor*KFactor/10000));");

INSERT INTO math_scripts VALUES( "CalculateChanceOfCastSuccess", "ChanceOfSuccess = (50-KFactor) + WaySkill/20 + (RelatedStat)/20;");

INSERT INTO math_scripts VALUES( "CalculateChanceOfResearchSuccess", "ChanceOfSuccess = WaySkill/10 + ( 10 / Spell:Realm ) * 7;");

INSERT INTO math_scripts VALUES( "CalculatePowerLevel", "PowerLevel = 1.0 + (WaySkill/10)*(1+(1+(200-WaySkill)/100)*(KFactor*KFactor/10000));");

INSERT INTO math_scripts VALUES( "SpellPractice", "PracticePoints = floor(10/(1 + MaxRealm - Realm))");

INSERT INTO math_scripts VALUES( "CalculateMaxCarryWeight", "MaxCarry =  ( Actor:getStatValue(0) );");

INSERT INTO math_scripts VALUES( "CalculateMaxCarryAmount", "MaxAmount =  750;");

INSERT INTO math_scripts VALUES( "MaxRealm", "MaxRealm = 1 + floor(WaySkill / 20);");

INSERT INTO math_scripts VALUES( "CalculateConsumeQuality", "QualityLevel = Quality/300;");

INSERT INTO math_scripts VALUES( "CalculateSkillCosts",
"
        NextRankCost = BaseCost +(1*(SkillRank/200));
        ZCost = (PracticeFactor/100.00)*NextRankCost;
        YCost = ((100.00-PracticeFactor)/100.00)*NextRankCost;
");

INSERT INTO math_scripts VALUES( "CalculateStatCosts",
"
        YCost = SkillRank / 10 + 10;
        ZCost = 0;
");

INSERT INTO math_scripts VALUES( "StaminaMove",
"
        Drain = (Speed/3) * (Weight/MaxWeight);
");

INSERT INTO math_scripts VALUES( "StaminaCombat",
"
        PhyDrain = 4 + rnd(3);
        MntDrain = 2 + rnd(5);
");

INSERT INTO math_scripts VALUES( "StaminaBase",
"
        BasePhy = (STR + END + AGI) / 3;
        BaseMen = (INT + WILL + CHA) / 3;
");

INSERT INTO math_scripts VALUES( "StaminaRatioWalk", 
"PStaminaRate = Actor:MaxPStamina/100*BaseRegenPhysical;
MStaminaRate = Actor:MaxMStamina/100*BaseRegenMental;");

INSERT INTO math_scripts VALUES( "StaminaRatioStill", 
"PStaminaRate = Actor:MaxPStamina/100*BaseRegenPhysical;
MStaminaRate = Actor:MaxMStamina/100*BaseRegenMental;");

INSERT INTO math_scripts VALUES( "StaminaRatioSit", 
"PStaminaRate = Actor:MaxPStamina*0.015*BaseRegenPhysical;
MStaminaRate = Actor:MaxMStamina*0.015*BaseRegenMental;");

INSERT INTO math_scripts VALUES( "StaminaRatioWork", 
"PStaminaRate = BaseRegenPhysical-6.0*(100-SkillMentalFactor)/100;
MStaminaRate = BaseRegenMental-6.0*(100-SkillMentalFactor)/100;");

INSERT INTO math_scripts VALUES( "CalculateMaxHP", "MaxHP = Actor:Will + Actor:Agility + Actor:Strength;");

INSERT INTO math_scripts VALUES( "CalculateMaxMana", "MaxMana = Actor:Will + Actor:Intelligence;");

INSERT INTO math_scripts VALUES( "LootModifierCostCap", "ModCap = MaxHP*10;");

INSERT INTO math_scripts VALUES( "Calculate Repair Rank","Result = if(Object:SalePrice > 300,Object:SalePrice/150,0);");

INSERT INTO math_scripts VALUES( "Calculate Repair Time",
"
        Result = Object:SalePrice/100;
        Factor = Worker:getSkillValue(Object:RequiredRepairSkill) / (Object:SalePrice/20);
        Result = Result / Factor;
        Result = if(Result < 20, 20, Result);
");

INSERT INTO math_scripts VALUES( "Calculate Repair Result",
"
        Factor = Worker:getSkillValue(Object:RequiredRepairSkill) / (Object:SalePrice/20);
        Result = ((Object:SalePrice/25) * Factor) * (rnd(1)+0.5);
");

INSERT INTO math_scripts VALUES( "Calculate Repair Quality",
"
ResultQ = if(Object:Quality+RepairAmount > Object:MaxQuality, Object:MaxQuality, Object:Quality+RepairAmount);
ResultMaxQ = Object:MaxQuality-(ResultQ-Object:Quality)*0.2;
ResultMaxQ = if(ResultMaxQ < 0, 0, ResultMaxQ);
ResultQ = if(ResultQ > ResultMaxQ, ResultMaxQ, ResultQ);
");

INSERT INTO math_scripts VALUES( "Calculate Repair Experience",
"
ResultPractice = 1;
ResultModifier = RepairAmount;
");

INSERT INTO math_scripts VALUES( "Calculate Mining Experience",
"
ResultPractice = if(Success, 1, 0);
ResultModifier = if(Success, 25, 2);
");

INSERT INTO math_scripts VALUES( "Calculate Trasformation Experience",
"
Exp = if(StartQuality < CurrentQuality, 2*(CurrentQuality-StartQuality), 0);
");

INSERT INTO math_scripts VALUES( "Calculate Skill Experience",
"
Exp = PracticePoints*Modifier;
");

INSERT INTO math_scripts VALUES( "CalculateFamiliarAffinity", "Affinity = Type + Lifecycle + AttackTool + AttackType;");

INSERT INTO math_scripts VALUES( "CalculateMaxPetTime", "MaxTime = 5 * 60 * 1000 * if(Skill,Skill,1);");
INSERT INTO math_scripts VALUES( "CalculateMaxPetRange", "MaxRange = 10 + Skill*10;" );
INSERT INTO math_scripts VALUES( "CalculatePetReact"," React = if(1+Skill>=Level,1,0);");

INSERT INTO math_scripts VALUES( "Calc Player Sketch Limits",
"
	IconScore = Actor:getSkillValue(64) + 5;
	PrimCount = Actor:getSkillValue(64) + 20;
");

INSERT INTO math_scripts VALUES( "Calc Item Price", "FinalPrice = Price + Quality + MaxQuality;");

INSERT INTO math_scripts VALUES( "Calc Item Sell Price", "FinalPrice = Price * 0.8;");

INSERT INTO math_scripts VALUES( "Calc Guild Account Level", "AccountLevel = 1 + log(log(TotalTrias));");

INSERT INTO math_scripts VALUES( "Calc Char Account Level", "AccountLevel = log(log(TotalTrias));");

INSERT INTO math_scripts VALUES( "Calc Bank Fee", "BankFee = 5.25 - (AccountLevel * 0.25);");

INSERT INTO math_scripts VALUES( "Calculate Mining Odds", "Total = Distance * Probability * Quality * Skill + 0.1");

INSERT INTO math_scripts VALUES( "Calc Item Merchant Price Buy", "Result = ItemPrice - CharData:getSkillValue(47)/10;");

INSERT INTO math_scripts VALUES( "Calc Item Merchant Price Sell", "Result = ItemPrice + CharData:getSkillValue(47)/10;");

INSERT INTO math_scripts VALUES( "Calculate Dynamic Experience", "Exp = 0;");



