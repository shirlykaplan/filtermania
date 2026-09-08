# Writen By May

## The students will get this file.

def read_maze_from_file(path):
	maze_file = open(path, "r")
	maze_board = maze_file.readlines()
	maze_file.close()

	# Tuple that represents the starting point.
	starting_point = [int(i) for i in maze_board[0][:-1].replace(' ','').replace('(','').replace(')','').split(',')]
	maze_board = maze_board[1:]

	for i in range(len(maze_board)):
		maze_board[i] = [int(j) for j in maze_board[i].replace(' ','')[:-1]]
	return (starting_point, maze_board)

"""
	This function recieves list of answers (strings) to that maze. For example:
	['ans1', 'ans2', 'ans3']
	when the answers are by the order of the mazes.
	'ans1' => the solution of maze0.
"""
def write_to_answers_file(list_of_answers):
	answers_file = open("answers.txt", "w")
	for answer in list_of_answers:
		answers_file.write(answer + '\n')
	answers_file.close()