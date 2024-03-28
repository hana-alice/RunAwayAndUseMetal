# Graphs
关于graph实现的思路。

# RenderGraph
 - [ ] TODO
# ResourceGraph
因为rhi层封装非常薄，鉴于vulkan后端Image指代memory resource，image view才是其他后端常用于frambuffer/attachment的资源，ResourceGraph会在用户声明资源的节点下创建一个默认的同名view资源，这样会影响一些ResourceGraph资源创建/回收的实现逻辑。
 - [ ] TO BE CONTINUED
# ShaderGraph
 - [ ] TODO
# TaskGraph
 - [ ] TODO
# SceneGraph
 - [ ] TODO