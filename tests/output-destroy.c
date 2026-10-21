/* A hidden tab must not acquire a reference to an output during its teardown. */
#include "compositor.h"

int
main(void)
{
	char runtime[] = "/tmp/vwl-output-destroy-XXXXXX";
	struct TestClient app;
	Monitor *removed, *kept;
	Client *active, *hidden;
	struct wlr_surface_output *entered;

	test_setup(runtime, "0", NULL);
	removed = add_monitor("TEST-REMOVE");
	kept = add_monitor("TEST-KEEP");
	assert(ipc_focus_virtual_output(firstvout(kept)) == 0);
	app = test_client_start(fullscreen_test_client, 20);
	client_command(app.control, 'r');
	client_command(app.control, 'u');
	active = wl_container_of(clients.next, active, link);
	client_command(app.control, 'n');
	hidden = wl_container_of(clients.prev, hidden, link);
	assert(active != hidden && active->mon == kept && hidden->mon == kept);
	setfloating(active, 0);
	setfloating(hidden, 0);
	setlayout(&(Arg){.v = &layouts[1]});
	focusclient(active, 1);
	client_command(app.control, 'c');
	assert(!hidden->scene->node.enabled);

	/* Moving the first screen shifts the second automatically. Hidden tabs
	 * retain their old coordinates, now covered by the first screen. */
	wlr_output_layout_add(output_layout, removed->wlr_output, kept->monitor_area.x, 0);
	assert(hidden->scene->node.x == removed->monitor_area.x);
	assert(!hidden->scene->node.enabled);
	wl_list_for_each(entered, &client_surface(hidden)->current_outputs, link)
		assert(entered->output != removed->wlr_output);

	/* Removing it used to briefly show the hidden tab on the dying output,
	 * add listeners too late for its destroy signal, and abort in wlroots. */
	wlr_output_destroy(removed->wlr_output);
	assert(active->mon == kept && hidden->mon == kept);
	assert(active->scene->node.enabled && !hidden->scene->node.enabled);
	focusclient(hidden, 1);
	client_command(app.control, 'c');
	assert(hidden->scene->node.enabled);
	assert(hidden->geom.x == kept->monitor_area.x);
	wl_list_for_each(entered, &client_surface(hidden)->current_outputs, link)
		assert(entered->output == kept->wlr_output);

	test_client_finish(app);
	test_cleanup(runtime);
	puts("output-destroy: hidden tabs survive monitor removal");
	return 0;
}
