import matplotlib.pyplot as plt
import matplotlib.patches as patches
import numpy as np

def draw_step(arcs, congested_links, step_name):
    fig, ax = plt.subplots(figsize=(12,2))
    
    # Draw nodes
    x = np.arange(16)
    for i in x:
        ax.add_patch(plt.Circle((i,0), 0.15, color='green'))
        ax.text(i, 0, str(i), ha='center', va='center', color='white')

    # Draw arrows
    for a, b, color in arcs:
        ax.annotate("",
            xy=(b,0.1),
            xytext=(a,0.1),
            arrowprops=dict(arrowstyle="-|>", lw=2, color=color,
            connectionstyle="arc3,rad=0.4")
        )

    # Draw congested links
    for (i,j) in congested_links:
        ax.plot([i,j], [-0.2,-0.2], color='red', lw=6)

    ax.set_xlim(-1,16)
    ax.set_ylim(-1,1)
    ax.axis('off')
    plt.title(step_name)
    plt.show()

# Example: Step 1 Recursive Doubling
arcs = [
    (0,2,'blue'), (1,3,'blue'), (4,6,'blue'), (5,7,'blue'),
    (8,10,'blue'), (9,11,'blue'), (12,14,'blue'), (13,15,'blue')
]
congested = [(1,2), (5,6)]
draw_step(arcs, congested, "Recursive Doubling – Step 1")
