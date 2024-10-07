vendors: [

random/seed 'clock

=>: func [msg data] [>> construct msg data]

talk-to: func [vendor locale /extern voice] [
    voice: none
    do select vendor/locale locale
    >> '^/'
    blk: vendor/intro

    while [block? blk] [    ; These lines drive the state machines.
        blk: do blk
    ]
]

music-shopping: does [music 4]

music-reset: [
    music 'reset
    quit
]

stats-reset: [
    music 'reset
    view-stats stats-party
    quit
]

enum: func [start list] [
    forall list [set first list ++ start]
]

; Matches StatsView in stats.h
stats-party: 0
enum 9 [
    stats-weapons   stats-armor     stats-equipment
    stats-items     stats-reagents  stats-mixtures
]

; Shared vendor variables.
shop: owner: price: quant: none
shop-vars: ['@' shop '%' owner '$' price '#' quant]

input-shop: func [msg choices] [
    ; Trim trailing newline (allows double braced strings to be used).
    if eq? '^/' last msg [msg: slice msg -1]

    >> construct msg shop-vars
    input-choice choices
]

type: key: name: none
item-vars: ['$' price '=' name]
item-fields: [type key name price]
inventory: ""
items: []

/*
    Build input-choice specification.

    Price_List is a block! of (Type Price) values.
    All_items is a block! of (Type Key Name Price) values.
*/
build-items: func [
    price_list check_money all_items
    /local it cost
][
    clear inventory
    clear items
    foreach [it cost] price_list [
        set item-fields find all_items it

        append inventory rejoin [uppercase key ' ' name '^/']

        append items mark-sol key
        append/block items reduce [
           to-set-word 'type  to-lit-word type
           to-set-word 'name  name
           to-set-word 'price cost
           check_money
        ]
    ]
]

weapons: context [
    intro: [
        view-stats stats-weapons
        music-shopping
        vo 0
        >> "^/^/^/"
        input-shop {{
            Welcome to
            @

            % says:
            Welcome friend!
            Art thou here to
            Buy or Sell? 
        }}
        [
            'b' [vo 2 >> "^/Very Good!" show_inventory]
            's' [vo 3 >> "^/Excellent! Which^/wouldst " you_sell]
            adieu
        ]
    ]

    end: stats-reset

    adieu: [
        vo 1
        => "^/% says:^/Fare thee well!^/" shop-vars
        end
    ]

    ; Buying

    show_inventory: [
        => "^/We Have:^/+Your^/Interest? "
            ['+' inventory]
        input-choice items
    ]

    ; check to see if the player has enough gold first
    check_money: [
        quant: 1
        either lt? party 'gold price [
            vo 4
            >> "^/You have not the funds for even one!^/"
            anything_else_buy
        ][
            >> '^/'
            => select description type shop-vars

            either gt? party 'gold mul price 2 [
                howmany_buy
            ][
                vo 5
                >> "^/Take it? "
                input-choice ['y' buy 'n' toobad adieu]
            ]
        ]
    ]

    toobad: [
        vo 6
        >> "^/Too bad."
        anything_else_buy
    ]

    description: [
        staff
        "We are the only staff makers in Britannia, yet sell them for only $gp.^/"
        dagger
        "We sell the most deadly of daggers, a bargain at only $gp each.^/"
        sling {{
            Our slings are made from only the finest guy and leather,
            'Tis yours for $gp.
        }}
        mace
        "These maces have a hardened shaft and a 5lb head fairly priced at $gp.^/"
        axe
        "Notice the fine workmanship on this axe, you'll agree $gp is a good price.^/"
        sword
        "The fine work on these swords will be the dread of thy foes, for $gp.^/"
        bow
        "Our bows are made of finest yew, and the arrows willow, a steal at $gp.^/"
        crossbow
        "Crossbows made by Iolo the Bard are the finest in the world, your for $gp.^/"
        oil {{
            Flasks of oil make great weapons and creates a wall of flame too.
            $gp each.
        }}
        halberd
        "A Halberd is a mighty weapon to attack over obstacles; a must and only $gp.^/"
        magic-axe
        "This magical axe can be thrown at thy enemy and will then return all for $gp.^/"
        magic-sword {{
            Magical swords such as these are rare indeed
            I will part with one for $gp.
        }}
        magic-bow
        "A magical bow will keep thy enemies far away or dead! A must for $gp!^/"
        magic-wand
        "This magic wand casts mighty blue bolts to strike down thy foes, $gp.^/"
    ]

    ; input how many to buy
    howmany_buy: [
        vo 7
        >> "^/How many would^/you like? "
        either quant: input-number 2 [buy] [anything_else_buy]
    ]

    ; buy the weapon
    buy: [
        either lt? party 'gold mul quant price [
            vo 8
            >> "^/I fear you have not the funds, perhaps something else."
            anything_else_buy
        ][
            pay price quant [
                add-items type quant
                vo 9
                => "^/% says: A fine choice!^/" shop-vars
                anything_else_buy
            ][
                check_money
            ]
        ]
    ]

    anything_else_buy: [
        >> "^/Anything^/else? "
        input-choice ['y' show_inventory 'n' adieu adieu]
    ]

    ; Selling

    you_sell: [
        >> "^/You sell: "
        either key: input-choice "bcdefghijklmnop" [
            ; Get item values of selected key char!.
            set item-fields prev find all-weapons key
            sell_weapon
        ][
            adieu
        ]
    ]

    sell_weapon: [
        price: div price 2
        switch party type [
            0 [
                vo 10
                >> "^/Thou dost not own that. What else might" you_sell
            ]
            1 [
                quant: 1
                vo 11
                => "^/I will give you $gp for that =.^/Deal? "
                    item-vars
                input-choice [
                    'y' sell
                    'n' [vo 28,153 >> "^/Hmmph. What else^/would " you_sell]
                    adieu
                ]
            ][
                => "^/How many =s^/would you wish^/to sell? "
                    item-vars
                either quant: input-number 2 [sell_many_offer] [sell]
            ]
        ]
    ]

    sell_many_offer: [
        price: mul price quant
        either gt? quant party type [
            vo 12
            >> "^/You don't have that many swine!^/"
            end
        ][
            vo 11
            => "^/I will give you $gp for them.^/Deal? "
                item-vars
            input-choice ['y' sell 'n' adieu adieu]
        ]
    ]

    sell: [
        remove-items type quant
        add-items 'gold price
        vo 13
        >> "^/Fine! What else?"
        you_sell
    ]

    ; Any weapon can be sold to any store, but each store doesn't carry
    ; every weapon.  This table provides how much gold to give when selling.

    all-weapons: [
        ;Type        Key  Name         Price
        staff        'b' "Staff"          20
        dagger       'c' "Dagger"          2
        sling        'd' "Sling"          25
        mace         'e' "Mace"          100
        axe          'f' "Axe"           225
        sword        'g' "Sword"         300
        bow          'h' "Bow"           250
        crossbow     'i' "Crossbow"      600
        oil          'j' "Flaming Oil"     5
        halberd      'k' "Halberd"       350
        magic-axe    'l' "Magic Axe"    1500
        magic-sword  'm' "Magic Sword"  2500
        magic-bow    'n' "Magic Bow"    2000
        magic-wand   'o' "Magic Wand"   5000
        mystic-sword 'p' "Mystic Sword" 7000
    ]

    stock: func [list] [
        build-items list check_money all-weapons
        append items 'adieu
    ]

    locale: [
        Britain [
            shop: "Windsor Weaponry"   owner: "Winston" voice: 28,139
            stock [staff 20 dagger 2 sling 25 sword 300]
        ]
        Jhelom [
            shop: "Willard's Weaponry" owner: "Willard" voice: 28,154
            stock [axe 225 sword 300 crossbow 600 halberd 350]
        ]
        Minoc [
            shop: "The Iron Works"     owner: "Peter"   voice: 28,168
            stock [mace 100 halberd 300 magic-axe 1500 magic-sword 2500]
        ]
        Trinsic [
            shop: "Dueling Weapons"    owner: "Jumar"   voice: 28,182
            stock [mace 100 axe 225 sword 300 bow 250]
        ]
        Buccaneers-Den [
            shop: "Hook's Arms"        owner: "Hook"    voice: 28,196
            stock [crossbow 600 oil 5 magic-bow 2000 magic-wand 5000]
        ]
        Vesper [
            shop: "Village Arms"       owner: "Wendy"   voice: 28,210
            stock [dagger 2 sling 25 bow 250 oil 5]
        ]
    ]
]

armor: context [
    intro: [
        view-stats stats-armor
        music-shopping
        >> "^/^/^/"
        vo 0
        input-shop {{
            Welcome to
            @

            % says:
            Welcome friend!
            Want to Buy or
            Sell? 
        }}
        [
            'b' [>> "^/Well then," show_inventory]
            's' [>> "^/What will" you_sell]
            adieu
        ]
    ]

    end: stats-reset

    adieu: [
        vo 1
        => "^/% says:^/Good Bye.^/" shop-vars
        end
    ]

    ; Buying

    show_inventory: [
        vo 2
        => "^/We've got:^/+^/What'll it^/be? "
            ['+' inventory]
        input-choice items
    ]

    ; check to see if the player has enough gold first
    check_money: [
        quant: 1
        either lt? party 'gold price [
            vo 3
            >> "^/You have not the funds for even one!^/"
            anything_else_buy
        ][
            >> '^/'
            => select description type shop-vars

            either gt? party 'gold mul price 2 [
                howmany_buy
            ][
                vo 4
                >> "^/Take it? "
                input-choice ['y' buy 'n' toobad adieu]
            ]
        ]
    ]

    toobad: [
        vo 5
        >> "^/Too bad."
        anything_else_buy
    ]

    description: [
        cloth
        "Cloth Armour is good for a tight budget, Fairly priced at $gp.^/"
        leather {{
            Leather Armour is both supple and strong, and costs a mere $gp.
            A Bargain!
        }}
        chain
        "Chail Mail is the armour used by more warriors than all others. Ours costs $gp.^/"
        plate
        "Full Plate armour is the ultimate in non-magical armour. Get yours for $gp.^/"
        magic-chain
        "Magic Armour is rare and expensive. This chain sells for $gp.^/"
        magic-plate
        "Magic Plate Armour is the best known protection. Only we have it.  Cost: $gp.^/"
    ]

    ; input how many to buy
    howmany_buy: [
        vo 6
        >> "^/How many would^/you like? "
        either quant: input-number 2 [buy] [anything_else_buy]
    ]

    ; buy the armor
    buy: [
        either lt? party 'gold mul quant price [
            vo 7
            >> "^/You don't have enough gold. Maybe something cheaper?^/"
            anything_else_buy
        ][
            pay price quant [
                add-items type quant
                vo 8
                => "^/% says: Good choice!^/" shop-vars
                anything_else_buy
            ][
                check_money
            ]
        ]
    ]

    anything_else_buy: [
        >> "^/Anything^/else? "
        input-choice ['y' show_inventory 'n' adieu adieu]
    ]

    ; Selling

    you_sell: [
        >> "^/You sell: "
        either key: input-choice "bcdefgh" [
            ; Get item values of selected key char!.
            set item-fields prev find all-armor key
            sell_armor
        ][
            adieu
        ]
    ]

    sell_armor: [
        price: div price 2
        switch party type [
            0 [
                vo 9
                >> "^/Come on, you^/don't own any."
                you_sell
            ]
            1 [
                quant: 1
                vo 10
                => "^/I will give you $gp for that =.^/Deal? "
                    item-vars
                input-choice ['y' sell 'n' no_sale adieu]
            ][
                vo 11
                => "^/How many =s^/would you wish^/to sell? "
                    item-vars
                either quant: input-number 2 [sell_many_offer] [sell]
            ]
        ]
    ]

    sell_many_offer: [
        price: mul price quant
        either gt? quant party type [
            vo 12
            >> "^/You don't have that many swine!^/"
            end
        ][
            vo 10
            => "^/I will give you $gp for them.^/Deal? "
                item-vars
            input-choice ['y' sell 'n' no_sale adieu]
        ]
    ]

    no_sale: [
        vo 28,82
        >> "^/Harumph. What else would "
        you_sell
    ]

    sell: [
        remove-items type quant
        add-items 'gold price
        vo 13
        >> "^/Fine! What else?"
        you_sell
    ]

    ; Any armor can be sold to any store, but each store doesn't carry
    ; every armor.  This table provides how much gold to give when selling.

    all-armor: [
        ;Type       Key  Name         Price
        cloth       'b' "Cloth"         50
        leather     'c' "Leather"      200
        chain       'd' "Chain Mail"   600
        plate       'e' "Plate Mail"  2000
        magic-chain 'f' "Magic Chain" 4000
        magic-plate 'g' "Magic Plate" 7000
        mystic-robe 'h' "Mystic Robe" 9000
    ]

    stock: func [list] [
        build-items list check_money all-armor
        append items 'adieu
    ]

    locale: [
        Britain [
            shop: "Winsdor Armour"   owner: "Winston" voice: 28,68
            stock [cloth 50 leather 200 chain 600]
        ]
        Jhelom [
            shop: "Valiant's Armour" owner: "Valiant" voice: 28,83
            stock [chain 600 plate 2000 magic-chain 4000 magic-plate 7000]
        ]
        Trinsic [
            shop: "Duelling Armour"  owner: "Jean"    voice: 28,97
            stock [cloth 50 chain 600 magic-chain 4000]
        ]
        Paws [
            shop: "Light Armour"     owner: "Pierre"  voice: 28,111
            stock [cloth 50 leather 200]
        ]
        Buccaneers-Den [
            shop: "Basic Armour"     owner: "Limpy"   voice: 28,125
            stock [cloth 50 leather 200 chain 600]
        ]
    ]
]

food: context [
    intro: [
        music-shopping
        vo 0
        => "Welcome to @^/^/% says: Good day, and Welcome friend.^/"
            shop-vars
        either lt? party 'gold price [
            vo 2
            >> "^/Come back when you have some money!^/"
            end
        ][
            vo 3
            >> "^/May I interest you in some rations? "
            input-choice ['y' ask 'n' adieu end]
        ]
    ]

    end: music-reset

    adieu: [vo 1 >> "^/Goodbye. Come again!^/" end]

    portion: 25     ; Number of food units.

    ask: [
        vo 4
        => "^/We have the best adventure rations, # for only $gp.^/"
            ['#' portion '$' price]
        howmany
    ]

    howmany: [
        vo 5
        => "^/How many packs of # would you like? "
            ['#' portion]
        either quant: input-number 3 [
            buy
        ][
            >> "^/Too bad. Maybe next time.^/"
            adieu
        ]
    ]

    buy: [
        pay price quant [
            add-items 'food mul portion quant
            >> "^/Thank you. "
            either lt? party 'gold price [
                vo 6
                >> "Come again!^/"
                end
            ][
                vo 7
                >> "Anything^/else? "
                input-choice ['y' howmany 'n' adieu adieu]
            ]
        ][
            ; DOS version exits with "Too bad..." line
            >> rejoin [
                "^/You can only afford " div party 'gold price " packs.^/"
            ]
            howmany
        ]
    ]

    locale: [
        Moonglow    [shop: "The Sage Deli"    owner: "Shaman"    price: 25
                     voice: 28,28]
        Britain     [shop: "Adventure Food"   owner: "Windrick"  price: 40
                     voice: 28,36]
        Yew         [shop: "The Dry Goods"    owner: "Donnar"    price: 35
                     voice: 28,44]
        Skara-Brae  [shop: "Food For Thought" owner: "Mintol"    price: 20
                     voice: 28,52]
        Paws        [shop: "The Market"       owner: "Max"       price: 30
                     voice: 28,60]
    ]
]

tavern: context [
    intro: [
        music-shopping
        ales_drunk: 0
        vo 0
        => "% says: Welcome to @" shop-vars
        whatll_it_be
    ]

    end: music-reset

    adieu: [vo 1 >> "See ya mate!^/" end]

    whatll_it_be: [
        input-shop "^/% says: What'll it be, Food er Ale? " [
            'f' [
                vo 2
                => "^/Our specialty is =, which costs $gp."
                    ['=' type '$' spec_price]
                how_many_plates
            ]
            'a' [
                either gt? ++ ales_drunk 2 [
                    vo 3
                    => "^/% says: Sorry, you seem to have too many. Bye!^/"
                        shop-vars
                    end     ; DOS version also does 'adieu.
                ][
                    price: 2
                    tip_total: 0
                    vo 4
                    >> "^/Here's a mug of our best.^/That'll be 2gp.^/You pay? "
                    either offer: input-number 2 [ale_buy] [adieu]
                ]
            ]
            end
        ]
    ]

    how_many_plates: [
        >> "^/How many plates would you^/like? "
        either quant: input-number 2 [
            pay spec_price quant [
                vo 6
                >> "Here ye arr.^/"
                add-items 'food quant
                something_else
            ][
                >> rejoin [
                    "Ya can only afford " div party 'gold spec_price
                    " plates.^/"
                ]
                vo 5
                how_many_plates
            ]
        ][
            adieu
        ]
    ]

    something_else: [
        >> "Somethin'^/else? "
        input-choice ['y' whatll_it_be 'n' adieu end]
    ]

    ales_drunk:
    spec_price:
    offer:
    topic:
    topics:
    tip_total: none

    ale_buy: [
        either lt? offer price [
            >> "^/Won't pay, eh.^/Ya scum, be gone^/fore ey call the^/guards!^/"
            end
        ][
            pay offer 1 [
                either gt? offer price [
                    tip_total: offer
                    vo 7
                    >> "^/What'd ya like to know friend?^/"
                    either topic: find topics input-text 0 [
                        price: second topic     ; Reuse price for topic.
                        topic: first topic
                        foggy
                    ][
                        vo 8
                        >> "^/'fraid I can't help ya there friend!^/"
                        something_else
                    ]
                ][
                    vo 9
                    something_else
                ]
            ][
                >> "^/It seems that you have not the gold. Good Day!^/"
                adieu
            ]
        ]
    ]

    sorry: [
        vo 10
        >> "^/Sorry, I could^/not help ya mate!^/"
        something_else
    ]

    foggy: [
        pay offer 1 [
            either ge? tip_total price [
                >> '^/'
                switch topic rumors
                >> '^/'
                something_else

               ;>> "^/Anythin'^/else? " input-choice [
               ;    'y' [>> "Here ye arr.^/" something_else] 'n' adieu adieu
               ;]
            ][
                vo 11
                >> "^/That subject is a bit foggy, perhaps more gold will refresh my memory. You^/give: "
                either offer: input-number 2 [
                    tip_total: add tip_total offer
                    foggy
                ][
                    sorry
                ]
            ]
        ][
            >> "^/Ye don't have that mate!^/"
            sorry
        ]
    ]

    rumors: [
        "black stone" [
            vo 17
            => {{
            % says: Ah, the Black Stone.
            Yes I've heard of it. But, the
            only one who knows where it
            lies is the wizard Merlin.
            }}
                shop-vars
        ]
        "sextant" [
            vo 16
            => {{
            % says: For navigation a Sextant is vital... Ask for item "D" in the Guild shops!
            }}
                shop-vars
        ]
        "white stone" [
            vo 15
            >> {{
            Now let me see... Yes it was the old Hermit...
            Sloven! He is tough to find,
            lives near Lock Lake I hear.
            }}
        ]
        "mandrake" [
            vo 14
            => {{
            % says: The last person I knew that had
            any Mandrake was an old alchemist
            named Calumny.
            }}
                shop-vars
        ]
        "skull" [
            vo 13
            => {{
            % says: If thou must know of that evilest of all things...
            find the beggar Jude. He is very very poor!
            }}
                shop-vars
        ]
        "nightshade" [
            vo 12
            => {{
            % says: Of Nightshade I know but this...
            Seek out Virgil or thou shalt miss! Try in Trinsic!
            }}
                shop-vars
        ]
    ]

    all-topics: [
        "black stone" 20
        "sextant"     30
        "white stone" 10
        "mandrake"    40
        "skull"       99
        "nightshade"  25
    ]

    locale: [
        Britain [
            shop: "Jolly Spirits" owner: "Sam" voice: 29,153
            type: "Lamb Chops" spec_price: 4
            topics: all-topics
        ]

        Jhelom [
            shop: "The Bloody Pub" owner: "Celestial" voice: 29,171
            type: "Dragon Tartar" spec_price: 2
            topics: skip all-topics 2
        ]

        Trinsic [
            shop: "The Keg Tap" owner: "Terran" voice: 29,188
            type: "Brown Beans" spec_price: 3
            topics: skip all-topics 4
        ]

        Paws [
            shop: "Folley Tavern" owner: "Greg 'n Rob" voice: 29,204
            type: "Folley Filet" spec_price: 2
            topics: skip all-topics 6
        ]

        Buccaneers-Den [
            shop: "Captain Black Tavern" owner: "The Cap'n" voice: 29,219
            type: "Dog Meat Pie" spec_price: 4
            topics: skip all-topics 8
        ]

        Vesper [
            shop: "Axe 'n Ale" owner: "Arron" voice: 29,233
            type: "Green Granukit" spec_price: 2
            topics: skip all-topics 10
        ]
    ]
]

reagents: context [
    intro: [
        music-shopping
        view-stats stats-reagents
        vo 0
        input-shop {{
            A blind woman turns to you and says: Welcome to @

            I am %
            Are you in need of Reagents? 
        }}
            ['y' [vo 2 >> "^/Very well," show_reagents] 'n' adieu adieu]
    ]

    end: stats-reset

    adieu: [
        vo 1
        => "^/% says:^/Perhaps another time then....^/and slowly turns away.^/"
            shop-vars
        end
    ]

    pindex:
    paying:
    prices: none

    inventory: {{
        A-Sulfurous Ash
        B-Ginseng
        C-Garlic
        D-Spider Silk
        E-Blood Moss
        F-Black Pearl
    }}

    items: [
        'a' [name: "Sulfur Ash"     type: 'ash      pindex: 1 how_many]
        'b' [name: "Ginseng"        type: 'ginseng  pindex: 2 how_many]
        'c' [name: "Garlic"         type: 'garlic   pindex: 3 how_many]
        'd' [name: "Spider Silk"    type: 'silk     pindex: 4 how_many]
        'e' [name: "Blood Moss"     type: 'moss     pindex: 5 how_many]
        'f' [name: "Black Pearl"    type: 'pearl    pindex: 6 how_many]
        adieu
    ]

    show_reagents: [
        => "^/I have^/+Your^/Interest: "
            ['+' inventory]
        input-choice items
    ]

    how_many: [
        price: pick prices pindex
        vo 3
        => "^/Very well, we sell = for $gp. How many would you^/like? "
            item-vars
        either quant: input-number 2 [you_pay] [i_see_then]
    ]

    you_pay: [
        price: mul price quant
        => "^/Very good, that will be $gp.  You pay: "
            shop-vars
        either paying: input-number 4 [
            pay paying 1 [
                vo 4
                >> "^/Very good. "
                add-items type quant
                karma [honesty justice honor] either lt? paying price
                    [either lt? d: sub price paying 12 -4 [div d -3]]
                    2
            ][
                vo 5
                >> "^/It seems you have not the gold! "
            ]
            anything_else
        ][
            i_see_then
        ]
    ]

    i_see_then: [
        vo 6
        >> "^/I see, then "
        anything_else
    ]

    anything_else: [
        >> "Anything^/else? "
        input-choice ['y' show_reagents 'n' adieu adieu]
    ]

    locale: [
        Moonglow [
            shop: "Magical Herbs"   owner: "Margot"  prices: 2,5,6,3,6,9
            voice: 28,0
        ]
        Skara-Brae [
            shop: "Herbs and Spice" owner: "Sasha"   prices: 2,4,9,6,4,8
            voice: 28,7
        ]
        Paws [
            shop: "The Magics"      owner: "Sheila"  prices: 3,4,2,9,6,7
            voice: 28,14
        ]
        Buccaneers-Den [
            shop: "Magic Mentar"    owner: "Shannon" prices: 6,7,9,9,9,1
            voice: 28,21
        ]
    ]
]

healer: context [
    intro: [
        music-shopping
        vo 0
        input-shop {{
            Welcome unto
            @

            % says:
            Peace and Joy be with you friend.
            Are you in need of help? 
        }}
            ['y' show_services 'n' give_blood adieu]
    ]

    end: music-reset

    adieu: [
        vo 1
        => "^/% says: May thy life be guarded by the powers of good.^/"
            shop-vars
        end
    ]

    give_blood: [
        either ge? pc-attr 1 hp 400 [
            vo 2
            >> "^/Art thou willing to give 100pts of thy blood to aid others? "
            input-choice [
                'y' [
                    damage-pc 1 100
                    karma sacrifice 5
                    >> "^/Thou art a great help. We are in dire need!^/"
                    game-wait 1000
                    adieu
                ]
                'n' [
                    karma sacrifice -5
                    adieu
                ]
                end
            ]
        ] end
    ]

    inventory: {{
        A-Curing
        B-Healing
        C-Resurrection
    }}

    pc:
    desc: none

    items: [
        'a' [desc: "A curing"     price: 100 type: 'cure      who_needs]
        'b' [desc: "A healing"    price: 200 type: 'fullheal  who_needs]
        'c' [desc: "Resurrection" price: 300 type: 'resurrect who_needs]
        give_blood ;?
    ]

    show_services: [
        vo 3
        => "^/% says: We can perform:^/+Your need: "
            ['%' owner '+' inventory]
        input-choice items
    ]

    who_needs: [
        either eq? 1 party 'members [
            pc: 1
            will_pay
        ][
            vo 4
            => "^/% asks:^/Who is in^/need? "
                shop-vars
            either pc: input-player [
                will_pay
            ][
                >> "No one?^/"
                more_help
            ]
        ]
    ]

    more_help: [
        vo 5
        input-shop "^/% asks: Do you need more help? "
            ['y' show_services 'n' give_blood give_blood]
    ]

    will_pay: [
        either pc-needs? pc type [
            => "^/+ will cost thee $gp.^/"
                ['+' desc '$' price]
            either lt? party 'gold price [
                vo 6
                >> "^/I see by thy purse that thou hast not enough gold. I cannot aid thee.^/"
                more_help
            ][
                vo 7
                >> "^/Wilt thou^/pay? "
                input-choice ['y' heal 'n' more_help end]
            ]
        ][
            >> select [
                cure      "Thou suffers not from Poison!^/"
                fullheal  "Thou art already quite healthy!^/"
                resurrect "Thou art not dead fool!^/"
            ] type
            more_help
        ]
    ]

    heal: [
        pay price 1 [
            pc-heal/magic pc type
            more_help
        ][
            more_help
        ]
    ]

    locale: [
        Britannia       [shop: "The Royal Healer"   owner: "Pendragon"
                         voice: 29,73]
        Moonglow        [shop: "The Healer"         owner: "Harmony"
                         voice: 29,81]
        Britain         [shop: "Wound Healing"      owner: "Celest"
                         voice: 29,89]
        Jhelom          [shop: "Heal and Health"    owner: "Triplet"
                         voice: 29,97]
        Yew             [shop: "Just Healing"       owner: "Justin"
                         voice: 29,105]
        Skara-Brae      [shop: "The Mystic Heal"    owner: "Spiran"
                         voice: 29,113]
        Lycaeum         [shop: "The Truth Healer"   owner: "Starfire"
                         voice: 29,121]
        Empath-Abbey    [shop: "The Love Healer"    owner: "Salle'"
                         voice: 29,129]
        Serpents-Hold   [shop: "The Courage Healer" owner: "Windwalker"
                         voice: 29,137]
        Cove            [shop: "The Healer Shop"    owner: "Quat"
                         voice: 29,145]
    ]
]

inn: context [
    intro: [
        music-shopping
        >> "The Innkeeper says: "
        either eq? 'horse party 'transport [
            vo 2
            >> "Get that horse out of here!!!^/"
            end
        ][
            vo 0
            input-shop
                "Welcome to @^/^/I am %.^/^/Are you in need of lodging? "
                ['y' ask 'n' adieu end]
        ]
    ]

    end: music-reset

    adieu: [
        vo 1
        => "^/% says: Then you have come to the wrong place!^/Good day.^/"
            shop-vars
        end
    ]

    ask:
    room-loc: none

    stay: [
        pay price 1 [
            cursor false
            vo 3
            >> "^/Very good.  Have^/a pleasant night.^/"
            relocate room-loc
            if eq? 1 random 4 [     ; 25% chance
                game-wait 1000
                vo 4
                >> "^/Oh, and don't mind the strange noises, it's only rats!^/"
            ]
            inn-sleep
        ][
            vo 5
            >> "^/If you can't pay, you can't stay! Good Bye.^/"
        ]
        end
    ]

    take-it?: func [msg] [
        vo 6
        >> '^/'
        >> msg
        >> "^/^/Take it? "
        input-choice [
            'y' stay
            'n' [vo 7 >> "^/You won't find a better deal in this towne!^/" end]
            end
        ]
    ]

    locale: [
        Moonglow [
            shop: "The Honest Inn" owner: "Scatu" price: 20 room-loc: 28,6
            voice: 29,0
            ask: [
                take-it? "We have a room with 2 beds that rents for 20gp."
            ]
        ]

        Britain [
            shop: "Britannia Manor" owner: "Jason" price: 15 room-loc: 29,6
            voice: 29,8
            ask: [
                take-it? "We have a modest sized room with 1 bed for 15 gp."
            ]
        ]

        Jhelom [
            shop: "The Inn of Ends" owner: "Smirk" price: 10 room-loc: 10,26
            voice: 29,16
            ask: [
                take-it?
                "We have a very secure room of modest size and 1 bed for 10gp."
            ]
        ]

        Minoc [
            shop: "Wayfarer's Inn" owner: "Estro" voice: 29,24
            ask: [
                vo 6
                >> "^/We have three rooms available,^/a 1, 2 and 3 bed room for 30, 60^/and 90gp each.^/1, 2 or 3^/beds? "
                ; Note: DOS version takes any input and repeats
                ;       "1, 2 or 3^/beds?" question when invalid.
                input-choice [
                    '1' [price: 30 room-loc: 2,6 stay]
                    '2' [price: 60 room-loc: 2,2 stay]
                    '3' [price: 90 room-loc: 8,2 stay]
                    end
                ]
            ]
        ]

        Trinsic [
            shop: "Honorable Inn" owner: "Zajac" price: 15 room-loc: 29,2
            voice: 29,32
            ask: [
                take-it? "We have a single bed room with a back door for 15gp."
            ]
        ]

        Skara-Brae [
            shop: "The Inn of the Spirits" owner: "Tyrone" price: 5
            room-loc: 28,11 voice: 29,40
            ask: [
                take-it? {{
                Unfortunately, I have but only a very small room with 1 bed:
                worse yet, it's haunted! If you do wish to stay it costs 5gp.
                }}
            ]
        ]

        Vesper [
            shop: "The Sleep Shop" owner: "Tymus" price: 1 room-loc: 25,23
            voice: 29,48
            ask: [
                take-it? "All we have is that cot over there. But it is comfortable, and only 1 gp."
            ]
        ]
    ]
]

guild: context [
    intro: [
        music-shopping
        view-stats stats-equipment
        vo 0
        input-shop {{
            Avast ye mate! Shure ye wishes to buy from ol'^/%?

            % says: Welcome to @.^/Like to see my goods? 
        }}
            ['y' show_goods 'n' adieu adieu]
    ]

    end: stats-reset

    adieu: [
        vo 1
        => "^/% says: See ya matie!^/" shop-vars
        end
    ]

    ; Sextants are hidden from the user.
    inventory: {{
        A-Torches
        B-Magic Gems
        C-Magic Keys
    }}

    items: [
        'a' [
            price:  50 quant: 5 type: 'torch
            will_buy? "^/I can give ya # long lasting Torches for a mere $gp."
        ]
        'b' [
            price:  60 quant: 5 type: 'gem
            will_buy? "^/I've got magical mapping Gems, # for only $gp."
        ]
        'c' [
            price:  60 quant: 6 type: 'key
            will_buy? "^/Magical Keys, 1 use each, a fair price at $gp for #."
        ]
        'd' [
            price: 900 quant: 1 type: 'sextant
            will_buy? "^/So...Ya want a Sextant...Well I gots one which I might part with fer $ gold!"
        ]
        adieu
    ]

    will_buy?: func [msg] [
        => msg shop-vars
        vo 2
        >> "^/^/Will ya buy? "
        input-choice ['y' buy 'n' [>> "^/Hmmm...Grmbl...^/" adieu] adieu]
    ]

    show_goods: [
        vo 3
        => "^/% says: Good Mate!^/Ya see I gots:^/+Wat'l it be? "
            ['%' owner '+' inventory]
        input-choice items
    ]

    buy: [
        pay price 1 [
            vo 4
            >> "^/Fine... fine...^/"
            add-items type quant

            input-shop "^/% says: See^/more? "
                ['y' show_goods 'n' adieu adieu]
        ][
            vo 5
            >> "^/What? Can't pay! Buzz off swine!^/"
            end
        ]
    ]

    locale: [
        Vesper [
            shop: "The Guild Shop" owner: "Long John Leary" voice: 29,56
        ]
        Buccaneers-Den [
            shop: "Pirate's Guild" owner: "One Eyed Willey" voice: 29,62
        ]
    ]
]

stable: context [
    intro: [
        music-shopping
        vo 0
        >> "Welcome friend!^/Can I interest thee in^/horses? "
        input-choice ['y' ask 'n' adieu end]
    ]

    end: music-reset

    adieu: [
        vo 1
        >> "^/A shame, thou looks like thou could use a good horse!^/"
        end
    ]

    ask: [
        vo 2
        input-shop "^/For only $g.p.^/Thou can have the best! Wilt thou buy? "
            ['y' buy 'n' adieu end]
    ]

    buy: [
        pay price 1 [
            add-item 'horse
            vo 3
            >> "^/Here, a better breed thou shalt not find ever!^/"
        ][
            vo 4
            >> "^/It seems thou hast not gold enough to pay!^/"
        ]
        end
    ]

    locale: [
        Paws [price: mul party 'members 100  voice: 29,68]
    ]
]

]
