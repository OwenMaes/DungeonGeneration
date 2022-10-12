Procedural Generation: Dungeon
---

What is procedural generation?

It is a method where algorithms are used to create data, 3d-models and all sorts of in-game content. It can make larger amounts of in game content in less time. A lot of games use procedural generation, for example the number 1 sold game 'Minecraft'. The world in Minecraft is completely procedurally generated. Even older games like "The Elder Scrolls II: Daggerfall" used this method, most of their world is also procedurally generated. This method is used by game developers for years, but it is still not the same as handmade content. A computer can not be as original and creative as a game developer. However, another argument for the method is making the in-game content less predictable. In many games you must grind the same dungeons repeatedly. Making these dungeons different every playthrough would make them more engaging for the player. In this research project I use some of these algorithms to create a randomly generated dungeon in Unreal.

### Binary space partitioning implementation
#### Step 1: Split the dungeon space with BSP
![ezgif com-gif-maker](https://user-images.githubusercontent.com/97401433/151252010-65f60948-2d18-4dfc-92de-b362efa7a31d.gif)

The dungeon space is split into multiple spaces by using the “Binary Space Partioning (BSP)” algorithm. This method recursively subdivides a space into two spaces. The dungeon space is the root of the tree, while each partitioned sub space is a leaf. The greater the split iterations the smaller the rooms. I also randomized the number of tiles each split and the direction: horizontal or vertical.

#### Step 2: Select the rooms and shrink them
![select](https://user-images.githubusercontent.com/97401433/151255303-27776002-146f-49a6-9103-476b64c73539.png)

Selecting the rooms is done by going through the root space, using inorder tree traversal. This method goes through the binary tree from the left to the right. During the traversal I check the current depth and if one of the two children of that space does not exist (nullptr). Which makes it a valid space to select as a dungeon room. All the dungeon rooms are shrunken after the traversal, so the rooms don’t stick together.

#### Step 3: Connect the rooms with corridors
![ezgif com-gif-maker (1)](https://user-images.githubusercontent.com/97401433/151260902-8102cfa1-7675-4bf3-81fc-02e4e3b83146.gif)

Connecting the rooms is probably the easiest part because the BSP has already taken care of that. You just create a corridor for each space you split by connecting the the center of both spaces. I created the corridors during the first step, by using a tmap in unreal. The map holds the space id of the left child (odd id) and the variables of the corridor itself (start and endpoint).  The start point is set when we are on odd index (left of tree) and the end point is set when we are on a even index.For example: the left child has the id ‘7’ and the right child id ‘8’, the left child can access the corridor by using it’s space index (7). The right child can access the corridor by their space id (8) minus one which is 7.

#### Step 4: Converting 2D to 3D in Unreal
![HighresScreenshot00000](https://user-images.githubusercontent.com/97401433/151262411-ebcd3d3a-736b-482d-aaa0-039d1fe2cb23.png)
![HighresScreenshot00005](https://user-images.githubusercontent.com/97401433/195446081-c99ab73c-15c1-410c-95e1-feea996b242f.png)

I use a TArray of tiles, each tile has the same size. I loop over the rooms, tile by tile and add them to the tile array. I do the same for each corridor. There are three tile types: Empty, Room and Corridor. I use the tile types to see where I can put walls and floors. I use the instanced static mesh component to quickly add and remove instances of each mesh.

### Random room placement implementation
#### Step 1: Creating random rooms
First start by creating random rooms with a different width/height. The centre of the rooms is a random position, in a radius around the spawn point of the dungeon. You can also easily make premade rooms and add them to the same array of rooms.
![Step1](https://user-images.githubusercontent.com/97401433/195439181-ef04eb99-a5ee-440f-8ca8-594811e90418.png)

#### Step 2: Seperating the rooms
Secondly you need to separate the rooms. I do this by looping trough all the rooms one by one. For each room I check which other rooms are overlapping with the current one. I calculate the direction from the other overlapping rooms, create an average direction of those and move the current room in that direction. The rooms all move until there are no rooms overlapping. You can make the rooms a bit bigger when checking the overlap, which creates some space between the rooms when they are separated.
![step2](https://user-images.githubusercontent.com/97401433/195441677-cfb0a9a9-a852-491a-9561-0adaa06faef5.gif)

#### Step 3: Connect the rooms
I loop through the rooms and connect them to the next one in the array, This ensures that every room is connected. I use A-Star search algorithm to make paths and connect the rooms. A-Star is generally a good method to use, however there are other search algorithms that can be more efficient for this. Another methodd is to use a Minimum Spanning Tree (MST) algorithm, which creates the least amount of connections and makes every room reachable.

![Step3](https://user-images.githubusercontent.com/97401433/195444798-ae836b2e-8761-46ce-aa37-2a472ec10bbc.gif)

#### Step 4: 2D to 3D
Again use the instanced static mesh componets of Unreal Engine 4 to spawn instanced of floors and walls.
![HighresScreenshot00000](https://user-images.githubusercontent.com/97401433/195446004-b8faaffc-0a65-431c-9b34-7e7b4a2c0937.png)
![HighresScreenshot00001](https://user-images.githubusercontent.com/97401433/195446150-b7cf383f-d00a-4247-84f8-4958b54ba205.png)

### Conclusion
Using BSP is a very fast way to create a randomized dungeon, i can repeatedly generate a new dungeon instantly in unreal. The general efficiency is O(1), but it depends on the author’s implementation. The number of splits, number of children, and the dimensions of the spaces all impact the efficiency. O(n+c+(w*h)). It does look visually less appealing than the RRP dungeon and it's not as easily customized because you can’t really use premade rooms. The BSP dungeon is also a big square, while with the RRP dungeon you have more freedom where you spawn them. In a circle, on a line, .... Overall, I prefer the RRP dungeon because the layout of the dungeon looks better and it’s also a very fast method to generate dungeons.

### References
-	https://www.gamedeveloper.com/programming/procedural-dungeon-generation-algorithm
-	https://en.wikipedia.org/wiki/Binary_space_partitioning
-	https://www.sbgames.org/sbgames2019/files/papers/ComputacaoFull/198359.pdf
-	https://www.researchgate.net/publication/316848565_Procedural_Dungeon_Generation_Analysis_and_Adaptation/download

