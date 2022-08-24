
void check_find() {
	struct Env *e1;
	struct Env *e1_child1;
	struct Env *e1_child2;
	struct Env *e1_child3;

	struct Env *e2;
	struct Env *e2_child;
	struct Env *e3;
	struct Env *e4;
	struct Env *e5;
	struct Env *e6;

	env_alloc(&e1, 0);
	env_alloc(&e1_child1, e1->env_id);
	env_alloc(&e1_child2, e1_child1->env_id);
	env_alloc(&e1_child3, e1_child1->env_id);
	env_alloc(&e4, e1_child2->env_id);
	env_alloc(&e5, e4->env_id);
	env_alloc(&e6, e1_child2->env_id);
	env_alloc(&e2, 0);
	env_alloc(&e2_child, e2->env_id);
	env_alloc(&e3, 0);

	find_sons(e1_child1->env_id);
	
void check_for_sum() {
	struct Env *env1;

	env_alloc(&env1, 0);
	fork(env1);
	fork(env1);
	fork(env1);

	struct Env *env1_child1;
	envid2env(env1->first_child->env_id, &env1_child1, 0);
	struct Env *env1_child2;
	envid2env(env1->first_child->next_brother->env_id, &env1_child2, 0);

	printf("total sum child is %d, correct -> 3\n", get_child_sum(env1->env_id));
	fork(env1_child1);
	fork(env1_child1);
	fork(env1_child1);
	fork(env1_child2);	

	printf("total sum child is %d, correct -> 7\n", get_child_sum(env1->env_id));
	struct Env *env1_grandson;
	envid2env(env1_child1->first_child->next_brother->env_id, &env1_grandson, 0);
	fork(env1_grandson);


	printf("total sum child is %d, correct -> 8\n", get_child_sum(env1->env_id));
/*	output(env1_child1->env_id);
	output(env1_grandson->env_id);

	printf("same father is %d\n", ENVX(check_same_father(env1_grandson->env_id, env1_child2->env_id)));
*/
	kill(env1_child1->env_id);
	printf("ok\n");
//	printf("env1 now has %d children\n", ENVX(env1->child_number));
	printf("total sum child is %d, correct -> 7\n", get_child_sum(env1->env_id));
	return ;
