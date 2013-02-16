
public class SizeKSubset {
	public static void main(String[] args) {
		//now, let's create a test case
		int[] array = {1,2,3,4};
		int K = 2;
		boolean[] used = new boolean[array.length];
		PrintAllSizeKSubset(array, used, 0, 0, 2);
		//expect, 12,13,14,23,24,34!
	}
	//now define the method header
	//as we discussed in slides, we need four supportive data for recursion
	//1. info whether each index is used
	//2. current focus index
	//3. current size
	//4. size K
	static void PrintAllSizeKSubset(int[] array, boolean[] used, 
			int startIndex, int currentSize, int K) {
		//firstly define when the recursion stops
		//case 1, the currentSize equals K, we print out
		if(currentSize==K) {
			for(int i=0; i<array.length; i++) {
				if(used[i])
					System.out.print(array[i]+" ");
			}
			System.out.println();//add a new line
			return;//do not forget to stop the recursion!
		}
		//case 2, focusIndex exceeds array length
		if(startIndex==array.length)
			return;
		//now it's the key recursion step as illustrated in our slides' example
		//firstly we select this index 
		used[startIndex] = true;
		PrintAllSizeKSubset(array,used,startIndex+1,currentSize+1,K);//key is +1
		//or 2nd option is to not using this index
		used[startIndex] = false;
		PrintAllSizeKSubset(array,used,startIndex+1,currentSize,K);
		//please notice index is increamented no matter we select or not!
	}
	
}


/**
* Please watch at http://www.youtube.com/user/ProgrammingInterview
* Contact: haimenboy@gmail.com
*
* Step by step to crack programming interview questions.
* 1. All questions were searched publicly from Google, Glassdoor, Careercup and StackOverflow.
* 2. All codes were written from scratch and links to download the source files are provided in each video's description. All examples were written in java, and tools I have used include Editplus, Eclipse and IntelliJ.
* 3. All videos were made without using any non-authorized material. All videos are silent sorry. Text comment is provided during coding as additional explanations.
* Thank you very much. 
*/
