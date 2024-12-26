// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2019 MediaTek Inc.

#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/of_device.h>

struct fsource_data {
	struct mutex lock;
	bool enabled;

	struct regulator *buck;
};

static const struct of_device_id fsource_of_match[] = {
	{ .compatible = "mtk-fsource" },
	{ }
};

MODULE_DEVICE_TABLE(of, fsource_of_match);

static ssize_t reg_show_state(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	struct fsource_data *data = dev_get_drvdata(dev);

	if (data->enabled)
		return sprintf(buf, "enabled\n");

	return sprintf(buf, "disabled\n");
}

static ssize_t reg_set_state(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	struct fsource_data *data = dev_get_drvdata(dev);
	bool enabled;
	int ret;

	/*
	 * sysfs_streq() doesn't need the \n's, but we add them so the strings
	 * will be shared with show_state(), above.
	 */
	if (sysfs_streq(buf, "enabled\n") || sysfs_streq(buf, "1"))
		enabled = true;
	else if (sysfs_streq(buf, "disabled\n") || sysfs_streq(buf, "0"))
		enabled = false;
	else {
		dev_err(dev, "Configuring invalid mode\n");
		return count;
	}

	mutex_lock(&data->lock);
	if (enabled != data->enabled) {
		if (enabled)
			ret = regulator_enable(data->buck);
		else
			ret = regulator_disable(data->buck);

		if (ret == 0)
			data->enabled = enabled;
		else
			dev_err(dev, "Failed to configure state: %d\n", ret);
	}
	mutex_unlock(&data->lock);

	return count;
}
static DEVICE_ATTR(state, 0644, reg_show_state, reg_set_state);

static struct attribute *attributes[] = {
	&dev_attr_state.attr,
	NULL,
};

static const struct attribute_group attr_group = {
	.attrs	= attributes,
};

static int fsource_probe(struct platform_device *pdev)
{
	struct fsource_data *drvdata;
	int ret;

	drvdata = devm_kzalloc(&pdev->dev,
			       sizeof(struct fsource_data),
			       GFP_KERNEL);
	if (drvdata == NULL)
		return -ENOMEM;

	mutex_init(&drvdata->lock);

	drvdata->buck = devm_regulator_get(&pdev->dev, "vfsource");
	if (IS_ERR(drvdata->buck)) {
		dev_err(&pdev->dev, "Failed to get supplies: %d\n", ret);
		return -ENODEV;
	}

	ret = sysfs_create_group(&pdev->dev.kobj, &attr_group);
	if (ret != 0)
		return ret;

	platform_set_drvdata(pdev, drvdata);

	return 0;
}

static int fsource_remove(struct platform_device *pdev)
{
	struct fsource_data *data = platform_get_drvdata(pdev);

	sysfs_remove_group(&pdev->dev.kobj, &attr_group);

	if (data->enabled && regulator_disable(data->buck))
		dev_err(&pdev->dev, "fail to disable vfsource power on remove\n");

	return 0;
}

static void fsource_shutdown(struct platform_device *pdev)
{
	struct fsource_data *data = platform_get_drvdata(pdev);

	sysfs_remove_group(&pdev->dev.kobj, &attr_group);

	if (data->enabled && regulator_disable(data->buck))
		dev_err(&pdev->dev, "fail to disable vfsource power on shutdown\n");

}

static struct platform_driver fsource_driver = {
	.driver		= {
		.name			= "mtk-fsource",
		.of_match_table		= of_match_ptr(fsource_of_match),
	},
	.probe		= fsource_probe,
	.remove		= fsource_remove,
	.shutdown	= fsource_shutdown,
};

module_platform_driver(fsource_driver);

MODULE_AUTHOR("Hsin-Hsiung Wang, MediaTek");
MODULE_DESCRIPTION("MediaTek Fsource Driver");
MODULE_LICENSE("GPL v2");

